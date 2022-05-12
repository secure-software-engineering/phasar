/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include <algorithm>
#include <cassert>
#include <iterator>

#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

#include "nlohmann/json.hpp"
#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace psr;

namespace psr {

const llvm::Function *
LLVMBasedCFG::getFunctionOf(const llvm::Instruction *Inst) const {
  return Inst->getFunction();
}

std::vector<const llvm::Instruction *>
LLVMBasedCFG::getPredsOf(const llvm::Instruction *I) const {
  if (!IgnoreDbgInstructions) {
    if (const auto *PrevInst = I->getPrevNode()) {
      return {PrevInst};
    }
  } else {
    if (const auto *PrevNonDbgInst =
            I->getPrevNonDebugInstruction(false /*Only debug instructions*/)) {
      return {PrevNonDbgInst};
    }
  }
  // If we do not have a predecessor yet, look for basic blocks which
  // lead to our instruction in question!

  std::vector<const llvm::Instruction *> Preds;
  std::transform(llvm::pred_begin(I->getParent()),
                 llvm::pred_end(I->getParent()), back_inserter(Preds),
                 [](const llvm::BasicBlock *BB) {
                   assert(BB && "BB under analysis was not well formed.");
                   const llvm::Instruction *Pred = BB->getTerminator();
                   if (llvm::isa<llvm::DbgInfoIntrinsic>(Pred)) {
                     Pred = Pred->getPrevNonDebugInstruction(
                         false /*Only debug instructions*/);
                   }
                   return Pred;
                 });

  return Preds;
}

std::vector<const llvm::Instruction *>
LLVMBasedCFG::getSuccsOf(const llvm::Instruction *I) const {
  // case we wish to consider LLVM's debug instructions
  if (!IgnoreDbgInstructions) {
    if (const auto *NextInst = I->getNextNode()) {
      return {NextInst};
    }
  } else if (const auto *NextNonDbgInst = I->getNextNonDebugInstruction(
                 false /*Only debug instructions*/)) {
    return {NextNonDbgInst};
  }
  if (const auto *Branch = llvm::dyn_cast<llvm::BranchInst>(I);
      Branch && isStaticVariableLazyInitializationBranch(Branch)) {
    // Skip the "already initialized" case, such that the analysis is always
    // aware of the initialized value.
    const auto *NextInst = &Branch->getSuccessor(0)->front();
    if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(NextInst)) {
      NextInst = NextInst->getNextNonDebugInstruction(false);
    }
    return {NextInst};
  }
  std::vector<const llvm::Instruction *> Successors;
  Successors.reserve(I->getNumSuccessors() + Successors.size());
  std::transform(
      llvm::succ_begin(I), llvm::succ_end(I), std::back_inserter(Successors),
      [IgnoreDbgInstructions{IgnoreDbgInstructions}](
          const llvm::BasicBlock *BB) {
        const llvm::Instruction *Succ = &BB->front();
        if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(Succ)) {
          Succ = Succ->getNextNonDebugInstruction(
              false /*Only debug instructions*/);
        }
        return Succ;
      });
  return Successors;
}

std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedCFG::getAllControlFlowEdges(const llvm::Function *Fun) const {
  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
      Edges;

  for (const auto &I : llvm::instructions(Fun)) {
    if (IgnoreDbgInstructions) {
      // Check for call to intrinsic debug function
      if (const auto *DbgCallInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
        if (DbgCallInst->getCalledFunction() &&
            DbgCallInst->getCalledFunction()->isIntrinsic() &&
            (DbgCallInst->getCalledFunction()->getName() ==
             "llvm.dbg.declare")) {
          continue;
        }
      }
    }

    auto Successors = getSuccsOf(&I);
    for (const auto *Successor : Successors) {
      Edges.emplace_back(&I, Successor);
    }
  }

  return Edges;
}

std::vector<const llvm::Instruction *>
LLVMBasedCFG::getAllInstructionsOf(const llvm::Function *Fun) const {
  std::vector<const llvm::Instruction *> Instructions;

  for (const auto &I : llvm::instructions(Fun)) {
    Instructions.push_back(&I);
  }

  return Instructions;
}

std::set<const llvm::Instruction *>
LLVMBasedCFG::getStartPointsOf(const llvm::Function *Fun) const {
  if (!Fun) {
    return {};
  }
  if (!Fun->isDeclaration()) {
    const auto *EntryInst = &Fun->front().front();
    if (IgnoreDbgInstructions && llvm::isa<llvm::DbgInfoIntrinsic>(EntryInst)) {
      return {EntryInst->getNextNonDebugInstruction(
          false /*Only debug instructions*/)};
    }
    return {EntryInst};
  }
  PHASAR_LOG_LEVEL(DEBUG, "Could not get starting points of '"
                              << Fun->getName()
                              << "' because it is a declaration");
  return {};
}

std::set<const llvm::Instruction *>
LLVMBasedCFG::getExitPointsOf(const llvm::Function *Fun) const {
  if (!Fun) {
    return {};
  }

  if (!Fun->isDeclaration()) {
    // A function can have more than one exit point
    std::set<const llvm::Instruction *> ExitPoints;
    auto ExitPointVector = psr::getAllExitPoints(Fun);

    for (const auto *ExitPoint : ExitPointVector) {
      ExitPoints.insert(ExitPoint);
    }

    return ExitPoints;
  }
  PHASAR_LOG_LEVEL(DEBUG, "Could not get exit points of '"
                              << Fun->getName() << "' which is declaration!");
  return {};
}

bool LLVMBasedCFG::isCallSite(const llvm::Instruction *Inst) const {
  return llvm::isa<llvm::CallBase>(Inst);
}

bool LLVMBasedCFG::isExitInst(const llvm::Instruction *Inst) const {
  return llvm::isa<llvm::ReturnInst>(Inst);
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction *Inst) const {
  return (Inst == &Inst->getFunction()->front().front());
}

bool LLVMBasedCFG::isFieldLoad(const llvm::Instruction *Inst) const {
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFieldStore(const llvm::Instruction *Inst) const {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction *Inst,
                                          const llvm::Instruction *Succ) const {
  // assert(false && "FallThrough not valid in LLVM IR");
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Inst)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == Succ;
    }
    return &B->getSuccessor(0)->front() == Succ;
  }
  return false;
}

bool LLVMBasedCFG::isBranchTarget(const llvm::Instruction *Inst,
                                  const llvm::Instruction *Succ) const {
  if (Inst->isTerminator()) {
    for (const auto *BB : llvm::successors(Inst->getParent())) {
      if (&BB->front() == Succ) {
        return true;
      }
    }
  }
  return false;
}

bool LLVMBasedCFG::isHeapAllocatingFunction(const llvm::Function *Fun) const {
  static const std::set<llvm::StringRef> HeapAllocatingFunctions = {
      "_Znwm", "_Znam", "malloc", "calloc", "realloc"};
  if (!Fun) {
    return false;
  }
  if (Fun->hasName() && HeapAllocatingFunctions.find(Fun->getName()) !=
                            HeapAllocatingFunctions.end()) {
    return true;
  }
  return false;
}

bool LLVMBasedCFG::isSpecialMemberFunction(const llvm::Function *Fun) const {
  return getSpecialMemberFunctionType(Fun) != SpecialMemberFunctionType::None;
}

SpecialMemberFunctionType
LLVMBasedCFG::getSpecialMemberFunctionType(const llvm::Function *Fun) const {
  if (!Fun) {
    return SpecialMemberFunctionType::None;
  }
  auto FunctionName = Fun->getName();
  // TODO this looks terrible and needs fix
  static const std::map<std::string, SpecialMemberFunctionType> Codes{
      {"C1", SpecialMemberFunctionType::Constructor},
      {"C2", SpecialMemberFunctionType::Constructor},
      {"C3", SpecialMemberFunctionType::Constructor},
      {"D0", SpecialMemberFunctionType::Destructor},
      {"D1", SpecialMemberFunctionType::Destructor},
      {"D2", SpecialMemberFunctionType::Destructor},
      {"aSERKS_", SpecialMemberFunctionType::CopyAssignment},
      {"aSEOS_", SpecialMemberFunctionType::MoveAssignment}};
  std::vector<std::pair<std::size_t, SpecialMemberFunctionType>> Found;
  std::size_t Blacklist = 0;
  auto It = Codes.begin();
  while (It != Codes.end()) {
    if (std::size_t Index = FunctionName.find(It->first, Blacklist)) {
      if (Index != std::string::npos) {
        Found.emplace_back(Index, It->second);
        Blacklist = Index + 1;
      } else {
        ++It;
        Blacklist = 0;
      }
    }
  }
  if (Found.empty()) {
    return SpecialMemberFunctionType::None;
  }

  // test if codes are in function name or type information
  bool NoName = true;
  for (auto Index : Found) {
    for (const auto *C = FunctionName.begin();
         C < FunctionName.begin() + Index.first; ++C) {
      if (isdigit(*C)) {
        short I = 0;
        while (isdigit(*(C + I))) {
          ++I;
        }
        std::string ST(C, C + I);
        if (Index.first <= std::distance(FunctionName.begin(), C) + stoul(ST)) {
          NoName = false;
          break;
        }
        C = C + *C;
      }
    }
    if (NoName) {
      return Index.second;
    }
    NoName = true;
  }
  return SpecialMemberFunctionType::None;
}

std::string LLVMBasedCFG::getStatementId(const llvm::Instruction *Inst) const {
  return llvm::cast<llvm::MDString>(
             Inst->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}

std::string LLVMBasedCFG::getFunctionName(const llvm::Function *Fun) const {
  return Fun->getName().str();
}

std::string
LLVMBasedCFG::getDemangledFunctionName(const llvm::Function *Fun) const {
  return llvm::demangle(getFunctionName(Fun));
}

void LLVMBasedCFG::print(const llvm::Function *F, llvm::raw_ostream &OS) const {
  OS << llvmIRToString(F);
}

nlohmann::json LLVMBasedCFG::getAsJson(const llvm::Function * /*F*/) const {
  return "";
}

[[nodiscard]] nlohmann::json
LLVMBasedCFG::exportCFGAsJson(const llvm::Function *F) const {
  nlohmann::json J;

  for (auto [From, To] : getAllControlFlowEdges(F)) {
    if (llvm::isa<llvm::UnreachableInst>(From)) {
      continue;
    }

    J.push_back({{"from", llvmIRToString(From)}, {"to", llvmIRToString(To)}});
  }

  return J;
}

[[nodiscard]] nlohmann::json
LLVMBasedCFG::exportCFGAsSourceCodeJson(const llvm::Function *F) const {
  nlohmann::json J;

  for (const auto &BB : *F) {
    assert(!BB.empty() && "Invalid IR: Empty BasicBlock");
    auto It = BB.begin();
    auto End = BB.end();
    auto From = getFirstNonEmpty(It, End);

    if (It == End) {
      continue;
    }
    ++It;
    // Edges inside the BasicBlock
    for (; It != End; ++It) {
      auto To = getFirstNonEmpty(It, End);
      if (To.empty()) {
        break;
      }

      J.push_back({{"from", From}, {"to", To}});

      From = std::move(To);
    }

    const auto *Term = BB.getTerminator();
    assert(Term && "Invalid IR: BasicBlock without terminating instruction!");

    const auto NumSuccessors = Term->getNumSuccessors();

    if (NumSuccessors != 0) {
      // Branch Edges

      for (const auto *Succ : llvm::successors(&BB)) {
        assert(Succ && !Succ->empty());

        auto To = getFirstNonEmpty(Succ);
        if (From != To) {
          J.push_back({{"from", From}, {"to", std::move(To)}});
        }
      }
    }
  }

  return J;
}

void from_json(const nlohmann::json &J,
               LLVMBasedCFG::SourceCodeInfoWithIR &Info) {
  from_json(J, static_cast<SourceCodeInfo &>(Info));
  J.at("IR").get_to(Info.IR);
}
void to_json(nlohmann::json &J,
             const LLVMBasedCFG::SourceCodeInfoWithIR &Info) {
  to_json(J, static_cast<const SourceCodeInfo &>(Info));
  J["IR"] = Info.IR;
}

auto LLVMBasedCFG::getFirstNonEmpty(llvm::BasicBlock::const_iterator &It,
                                    llvm::BasicBlock::const_iterator End)
    -> SourceCodeInfoWithIR {
  assert(It != End);

  const auto *Inst = &*It;
  auto Ret = getSrcCodeInfoFromIR(Inst);

  // Assume, we aren't skipping relevant calls here

  while ((Ret.empty() || It->isDebugOrPseudoInst()) && ++It != End) {
    Inst = &*It;
    Ret = getSrcCodeInfoFromIR(Inst);
  }

  return {Ret, llvmIRToString(Inst)};
}

auto LLVMBasedCFG::getFirstNonEmpty(const llvm::BasicBlock *BB)
    -> SourceCodeInfoWithIR {
  auto It = BB->begin();
  return getFirstNonEmpty(It, BB->end());
}

} // namespace psr
