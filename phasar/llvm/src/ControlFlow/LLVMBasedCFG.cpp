/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

#include "phasar/ControlFlow/SpecialMemberFunctionType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/IntrinsicInst.h"

#include <string>

namespace psr {

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getFunctionOfImpl(
    n_t Inst) const noexcept -> f_t {
  assert(Inst != nullptr);
  return Inst->getFunction();
}

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getPredsOfImpl(n_t I) const
    -> llvm::SmallVector<n_t, 2> {
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

  llvm::SmallVector<n_t, 2> Preds;
  std::transform(llvm::pred_begin(I->getParent()),
                 llvm::pred_end(I->getParent()), std::back_inserter(Preds),
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

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getSuccsOfImpl(n_t I) const
    -> llvm::SmallVector<n_t, 2> {
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

  llvm::SmallVector<n_t, 2> Successors;
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

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getAllControlFlowEdgesImpl(
    f_t Fun) const -> std::vector<std::pair<n_t, n_t>> {
  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
      Edges;

  for (const auto &I : llvm::instructions(Fun)) {
    if (IgnoreDbgInstructions) {
      // Check for call to intrinsic debug function
      if (llvm::isa<llvm::DbgInfoIntrinsic>(&I)) {
        continue;
      }
    }

    auto Successors = this->getSuccsOf(&I);
    for (const auto *Successor : Successors) {
      Edges.emplace_back(&I, Successor);
    }
  }

  return Edges;
}

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getStartPointsOfImpl(f_t Fun) const
    -> llvm::SmallVector<n_t, 2> {
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
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedCFG",
                       "Could not get starting points of '"
                           << Fun->getName()
                           << "' because it is a declaration");
  return {};
}

template <typename Derived>
auto detail::LLVMBasedCFGImpl<Derived>::getExitPointsOfImpl(f_t Fun) const
    -> llvm::SmallVector<n_t, 2> {
  if (!Fun) {
    return {};
  }

  if (!Fun->isDeclaration()) {
    // A function can have more than one exit point
    return psr::getAllExitPoints(Fun);
  }
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedCFG",
                       "Could not get exit points of '"
                           << Fun->getName() << "' which is declaration!");
  return {};
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isStartPointImpl(
    n_t Inst) const noexcept {
  auto FirstInst = &Inst->getFunction()->front().front();
  if (Inst == FirstInst) {
    return true;
  }
  return llvm::isa<llvm::DbgInfoIntrinsic>(FirstInst) &&
         Inst == FirstInst->getNextNonDebugInstruction(false);
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isFieldLoadImpl(
    n_t Inst) const noexcept {
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isFieldStoreImpl(
    n_t Inst) const noexcept {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isFallThroughSuccessorImpl(
    n_t Inst, n_t Succ) const noexcept {
  // assert(false && "FallThrough not valid in LLVM IR");
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Inst)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == Succ;
    }
    return &B->getSuccessor(0)->front() == Succ;
  }
  return false;
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isBranchTargetImpl(
    n_t Inst, n_t Succ) const noexcept {
  if (Inst->isTerminator()) {
    for (const auto *BB : llvm::successors(Inst->getParent())) {
      if (&BB->front() == Succ) {
        return true;
      }
    }
  }
  return false;
}

template <typename Derived>
bool detail::LLVMBasedCFGImpl<Derived>::isHeapAllocatingFunctionImpl(
    f_t Fun) const {
  return llvm::StringSwitch<bool>(Fun->getName())
      .Cases("_Znwm", "_Znam", "malloc", "calloc", "realloc", true)
      .Default(false);
}

template <typename Derived>
SpecialMemberFunctionType
detail::LLVMBasedCFGImpl<Derived>::getSpecialMemberFunctionTypeImpl(
    f_t Fun) const {
  if (!Fun) {
    return SpecialMemberFunctionType::None;
  }
  llvm::StringRef FunctionName = Fun->getName();
  /// TODO: this looks terrible and needs fix
  static constexpr std::pair<llvm::StringLiteral, SpecialMemberFunctionType>
      Codes[] = {{"C1", SpecialMemberFunctionType::Constructor},
                 {"C2", SpecialMemberFunctionType::Constructor},
                 {"C3", SpecialMemberFunctionType::Constructor},
                 {"D0", SpecialMemberFunctionType::Destructor},
                 {"D1", SpecialMemberFunctionType::Destructor},
                 {"D2", SpecialMemberFunctionType::Destructor},
                 {"aSERKS_", SpecialMemberFunctionType::CopyAssignment},
                 {"aSEOS_", SpecialMemberFunctionType::MoveAssignment}};
  llvm::SmallVector<std::pair<std::size_t, SpecialMemberFunctionType>> Found;
  std::size_t Blacklist = 0;
  const auto *It = std::begin(Codes);
  while (It != std::end(Codes)) {
    if (std::size_t Index = FunctionName.find(It->first, Blacklist)) {
      if (Index != llvm::StringRef::npos) {
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

template <typename Derived>
std::string
detail::LLVMBasedCFGImpl<Derived>::getStatementIdImpl(n_t Inst) const {
  return getMetaDataID(Inst);
}

template <typename Derived>
std::string
detail::LLVMBasedCFGImpl<Derived>::getDemangledFunctionNameImpl(f_t Fun) const {
  return llvm::demangle(Fun->getName().str());
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

struct SourceCodeInfoWithIR : public SourceCodeInfo {
  std::string IR;
};

static void to_json(nlohmann::json &J, const SourceCodeInfoWithIR &Info) {
  to_json(J, static_cast<const SourceCodeInfo &>(Info));
  J["IR"] = Info.IR;
}

static auto getFirstNonEmpty(llvm::BasicBlock::const_iterator &It,
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

static auto getFirstNonEmpty(const llvm::BasicBlock *BB)
    -> SourceCodeInfoWithIR {
  auto It = BB->begin();
  return getFirstNonEmpty(It, BB->end());
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

template class detail::LLVMBasedCFGImpl<LLVMBasedCFG>;
template class detail::LLVMBasedCFGImpl<LLVMBasedBackwardCFG>;
} // namespace psr
