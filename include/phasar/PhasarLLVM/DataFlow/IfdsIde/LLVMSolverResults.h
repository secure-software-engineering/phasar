/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMSOLVERRESULTS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMSOLVERRESULTS_H

#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/ErrorHandling.h"

namespace psr::detail {

template <typename Derived, typename N, typename D, typename L>
template <typename NTy>
auto SolverResultsBase<Derived, N, D, L>::resultsAtInLLVMSSA(
    ByConstRef<n_t> Stmt, bool AllowOverapproximation, bool StripZero) ->
    typename std::enable_if_t<
        std::is_same_v<std::decay_t<std::remove_pointer_t<NTy>>,
                       llvm::Instruction>,
        std::unordered_map<d_t, l_t>> {
  std::unordered_map<d_t, l_t> Result = [this, Stmt, AllowOverapproximation]() {
    if (Stmt->getType()->isVoidTy()) {
      return self().Results.row(Stmt);
    }
    if (!Stmt->getNextNode()) {
      auto GetStartRow = [this](const llvm::BasicBlock *BB) -> decltype(auto) {
        const auto *First = &BB->front();
        if (llvm::isa<llvm::DbgInfoIntrinsic>(First)) {
          First = First->getNextNonDebugInstruction();
        }
        return self().Results.row(First);
      };

      // We have reached the end of a BasicBlock. If there is a successor BB
      // that only has one predecessor, we are lucky and can just take results
      // from there
      for (const llvm::BasicBlock *Succ : llvm::successors(Stmt)) {
        if (Succ->hasNPredecessors(1)) {
          return GetStartRow(Succ);
        }
      }

      if (!AllowOverapproximation) {
        llvm::report_fatal_error("[resultsAtInLLVMSSA]: Cannot precisely "
                                 "collect the results at instruction " +
                                 llvm::Twine(llvmIRToString(Stmt)));
      }

      // There is no successor with only one predecessor.
      // All we can do is merge the results from all successors to get a sound
      // overapproximation. This is not optimal and may be replaced in the
      // future.
      PHASAR_LOG_LEVEL(WARNING, "[resultsAtInLLVMSSA]: Cannot precisely "
                                "collect the results at instruction "
                                    << llvmIRToString(Stmt)
                                    << ". Use a sound, but potentially "
                                       "imprecise overapproximation");
      std::unordered_map<d_t, l_t> Ret;
      for (const llvm::BasicBlock *Succ : llvm::successors(Stmt)) {
        const auto &Row = GetStartRow(Succ);
        for (const auto &[Fact, Value] : Row) {
          auto [It, Inserted] = Ret.try_emplace(Fact, Value);
          if (!Inserted && Value != It->second) {
            if constexpr (HasJoinLatticeTraits<l_t>) {
              It->second = JoinLatticeTraits<l_t>::join(It->second, Value);
            } else {
              // We have no way of correctly merging, so set the value to the
              // default constructed l_t hoping it marks BOTTOM.
              It->second = l_t();
            }
          }
        }
      }
      return Ret;
    }
    assert(Stmt->getNextNode() && "Expected to find a valid successor node!");
    return self().Results.row(Stmt->getNextNode());
  }();
  if (StripZero) {
    Result.erase(self().ZV);
  }
  return Result;
}

template <typename Derived, typename N, typename D, typename L>
template <typename NTy>
auto SolverResultsBase<Derived, N, D, L>::resultAtInLLVMSSA(
    ByConstRef<n_t> Stmt, d_t Value, bool AllowOverapproximation) ->
    typename std::enable_if_t<
        std::is_same_v<std::decay_t<std::remove_pointer_t<NTy>>,
                       llvm::Instruction>,
        l_t> {
  if (Stmt->getType()->isVoidTy()) {
    return self().Results.get(Stmt, Value);
  }
  if (!Stmt->getNextNode()) {
    auto GetStartVal = [this,
                        &Value](const llvm::BasicBlock *BB) -> decltype(auto) {
      const auto *First = &BB->front();
      if (llvm::isa<llvm::DbgInfoIntrinsic>(First)) {
        First = First->getNextNonDebugInstruction();
      }
      return self().Results.get(First, Value);
    };

    // We have reached the end of a BasicBlock. If there is a successor BB
    // that only has one predecessor, we are lucky and can just take results
    // from there
    for (const llvm::BasicBlock *Succ : llvm::successors(Stmt)) {
      if (Succ->hasNPredecessors(1)) {
        return GetStartVal(Succ);
      }
    }

    if (!AllowOverapproximation) {
      llvm::report_fatal_error("[resultsAtInLLVMSSA]: Cannot precisely "
                               "collect the results at instruction " +
                               llvm::Twine(llvmIRToString(Stmt)));
    }

    // There is no successor with only one predecessor.
    // All we can do is merge the results from all successors to get a sound
    // overapproximation. This is not optimal and may be replaced in the
    // future.
    PHASAR_LOG_LEVEL(WARNING, "[resultAtInLLVMSSA]: Cannot precisely "
                              "collect the results at instruction "
                                  << *Stmt
                                  << ". Use a sound, but potentially "
                                     "imprecise overapproximation");
    auto It = llvm::succ_begin(Stmt);
    auto End = llvm::succ_end(Stmt);
    l_t Ret{};
    if (It != End) {
      Ret = GetStartVal(*It);
      for (++It; It != End; ++It) {
        const auto &Val = GetStartVal(*It);
        if constexpr (HasJoinLatticeTraits<l_t>) {
          Ret = JoinLatticeTraits<l_t>::join(Ret, Val);
          if (Ret == JoinLatticeTraits<l_t>::bottom()) {
            break;
          }
        } else {
          if (Ret != Val) {
            // We have no way of correctly merging, so set the value to the
            // default constructed l_t hoping it marks BOTTOM.
            return {};
          }
        }
      }
    }
    return Ret;
  }
  assert(Stmt->getNextNode() && "Expected to find a valid successor node!");
  return self().Results.get(Stmt->getNextNode(), Value);
}
} // namespace psr::detail

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMSOLVERRESULTS_H
