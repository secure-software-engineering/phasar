#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"

namespace llvm {
class Function;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

/// Alias-analysis result for the Andersen-style OTF points-to analysis.
///
/// Two values may-alias iff their points-to sets share at least one abstract
/// object.  Satisfies \c UnionFindAAResult so it can be wrapped by
/// \c LLVMUnionFindAliasIterator.
struct AndersenOTFResult {
  TypedVector<ValueId, RawAliasSet<ValueId>> AliasSets;
  LLVMBasedCallGraph CG;

  [[nodiscard]] static constexpr bool isCached() noexcept { return true; }
  [[nodiscard]] constexpr size_t size() const noexcept {
    return AliasSets.size();
  }

  [[nodiscard]] RawAliasSet<ValueId>
  getRawAliasSet(ValueId Var) const noexcept {
    if (!AliasSets.inbounds(Var)) {
      return {};
    }
    return AliasSets[Var];
  }

  [[nodiscard]] bool mayAlias(ValueId Var1, ValueId Var2) const noexcept {
    if (Var1 == Var2) {
      return true;
    }
    if (!AliasSets.inbounds(Var1)) {
      return false;
    }
    return AliasSets[Var1].contains(Var2);
  }
};

static_assert(UnionFindAAResult<AndersenOTFResult>);

/// Andersen-style inclusion-based points-to analysis that co-refines the call
/// graph and points-to sets in a single fixpoint.
///
/// Unlike the staged pipeline (resolver → PA), this solver owns its own
/// function-worklist loop: direct calls add callees immediately; indirect
/// calls are resolved as \c pts(fp) grows.
///
/// Phase 1: context- and field-insensitive.
class AndersenOTFSolver {
public:
  explicit AndersenOTFSolver(const LLVMProjectIRDB &IRDB,
                             llvm::ArrayRef<const llvm::Function *> Entries,
                             ValueCompressor<PAGVariable> &VC,
                             Soundness S = Soundness::Soundy) noexcept;

  /// Run the full OTF fixpoint and return the alias-analysis result.
  [[nodiscard]] AndersenOTFResult solve();

private:
  struct SolverData;

  NonNullPtr<const LLVMProjectIRDB> IRDB;
  llvm::ArrayRef<const llvm::Function *> Entries;
  NonNullPtr<ValueCompressor<PAGVariable>> VC;
  Soundness S;
};

// ---- Factory functions ------------------------------------------------

/// Runs the Andersen OTF fixpoint and returns the raw alias-analysis result
/// (no LLVM-value wrapping).  If \p VC is null, a fresh one is allocated.
[[nodiscard]] AndersenOTFResult
computeAndersenOTFRaw(const LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<const llvm::Function *> EntryPoints,
                      MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                      Soundness S = Soundness::Soundy);

/// Runs the Andersen OTF fixpoint and returns an \c LLVMUnionFindAliasIterator
/// that implements \c IsLLVMAliasIterator.
[[nodiscard]] LLVMUnionFindAliasIterator<AndersenOTFResult>
computeAndersenOTF(const LLVMProjectIRDB &IRDB,
                   llvm::ArrayRef<const llvm::Function *> EntryPoints,
                   MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                   Soundness S = Soundness::Soundy);

} // namespace psr
