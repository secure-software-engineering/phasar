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

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using namespace std;
using namespace psr;

namespace psr {

const llvm::Function *
LLVMBasedCFG::getFunctionOf(const llvm::Instruction *Stmt) const {
  return Stmt->getFunction();
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getPredsOf(const llvm::Instruction *I) const {
  vector<const llvm::Instruction *> Preds;
  if (!IgnoreDbgInstructions) {
    if (I->getPrevNode()) {
      Preds.push_back(I->getPrevNode());
    }
  } else {
    if (I->getPrevNonDebugInstruction()) {
      Preds.push_back(I->getPrevNonDebugInstruction());
    }
  }
  // If we do not have a predecessor yet, look for basic blocks which
  // lead to our instruction in question!
  if (Preds.empty()) {
    std::transform(llvm::pred_begin(I->getParent()),
                   llvm::pred_end(I->getParent()), back_inserter(Preds),
                   [](const llvm::BasicBlock *BB) {
                     assert(BB && "BB under analysis was not well formed.");
                     return BB->getTerminator();
                   });
  }
  return Preds;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getSuccsOf(const llvm::Instruction *I) const {
  vector<const llvm::Instruction *> Successors;
  // case we wish to consider LLVM's debug instructions
  if (!IgnoreDbgInstructions) {
    if (I->getNextNode()) {
      Successors.push_back(I->getNextNode());
    }
  } else {
    if (I->getNextNonDebugInstruction()) {
      Successors.push_back(I->getNextNonDebugInstruction());
    }
  }
  if (I->isTerminator()) {
    Successors.reserve(I->getNumSuccessors() + Successors.size());
    std::transform(llvm::succ_begin(I), llvm::succ_end(I),
                   back_inserter(Successors),
                   [](const llvm::BasicBlock *BB) { return &BB->front(); });
  }
  return Successors;
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedCFG::getAllControlFlowEdges(const llvm::Function *Fun) const {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
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
  }
  return Edges;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getAllInstructionsOf(const llvm::Function *Fun) const {
  vector<const llvm::Instruction *> Instructions;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

std::set<const llvm::Instruction *>
LLVMBasedCFG::getStartPointsOf(const llvm::Function *Fun) const {
  if (!Fun) {
    return {};
  }
  if (!Fun->isDeclaration()) {
    return {&Fun->front().front()};
  } else {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Could not get starting points of '"
                  << Fun->getName().str() << "' because it is a declaration");
    return {};
  }
}

std::set<const llvm::Instruction *>
LLVMBasedCFG::getExitPointsOf(const llvm::Function *Fun) const {
  if (!Fun) {
    return {};
  }
  if (!Fun->isDeclaration()) {
    return {&Fun->back().back()};
  } else {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Could not get exit points of '" << Fun->getName().str()
                  << "' which is declaration!");
    return {};
  }
}

bool LLVMBasedCFG::isCallStmt(const llvm::Instruction *Stmt) const {
  return llvm::isa<llvm::CallInst>(Stmt) || llvm::isa<llvm::InvokeInst>(Stmt);
}

bool LLVMBasedCFG::isExitStmt(const llvm::Instruction *Stmt) const {
  return llvm::isa<llvm::ReturnInst>(Stmt);
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction *Stmt) const {
  return (Stmt == &Stmt->getFunction()->front().front());
}

bool LLVMBasedCFG::isFieldLoad(const llvm::Instruction *Stmt) const {
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Stmt)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFieldStore(const llvm::Instruction *Stmt) const {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Stmt)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction *Stmt,
                                          const llvm::Instruction *Succ) const {
  // assert(false && "FallThrough not valid in LLVM IR");
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Stmt)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == Succ;
    } else {
      return &B->getSuccessor(0)->front() == Succ;
    }
  }
  return false;
}

bool LLVMBasedCFG::isBranchTarget(const llvm::Instruction *Stmt,
                                  const llvm::Instruction *Succ) const {
  if (Stmt->isTerminator()) {
    for (const auto *BB : llvm::successors(Stmt->getParent())) {
      if (&BB->front() == Succ) {
        return true;
      }
    }
  }
  return false;
}

bool LLVMBasedCFG::isHeapAllocatingFunction(const llvm::Function *Fun) const {
  static const std::set<std::string> HeapAllocatingFunctions = {
      "_Znwm", "_Znam", "malloc", "calloc", "realloc"};
  if (!Fun) {
    return false;
  }
  if (Fun->hasName() && HeapAllocatingFunctions.find(Fun->getName().str()) !=
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
    for (auto C = FunctionName.begin(); C < FunctionName.begin() + Index.first;
         ++C) {
      if (isdigit(*C)) {
        short I = 0;
        while (isdigit(*(C + I))) {
          ++I;
        }
        std::string ST(C, C + I);
        if (Index.first <= std::distance(FunctionName.begin(), C) + stoul(ST)) {
          NoName = false;
          break;
        } else {
          C = C + *C;
        }
      }
    }
    if (NoName) {
      return Index.second;
    } else {
      NoName = true;
    }
  }
  return SpecialMemberFunctionType::None;
}

string LLVMBasedCFG::getStatementId(const llvm::Instruction *Stmt) const {
  return llvm::cast<llvm::MDString>(
             Stmt->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}

string LLVMBasedCFG::getFunctionName(const llvm::Function *Fun) const {
  return Fun->getName().str();
}

std::string
LLVMBasedCFG::getDemangledFunctionName(const llvm::Function *Fun) const {
  return cxxDemangle(getFunctionName(Fun));
}

void LLVMBasedCFG::print(const llvm::Function *F, std::ostream &OS) const {
  OS << llvmIRToString(F);
}

nlohmann::json LLVMBasedCFG::getAsJson(const llvm::Function *F) const {
  return "";
}

} // namespace psr
