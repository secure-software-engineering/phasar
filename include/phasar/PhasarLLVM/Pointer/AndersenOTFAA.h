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
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"

#include <cstdint>
#include <string>
#include <vector>

namespace llvm {
class Function;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

/// Opt-in call-string context-sensitivity for \c AndersenOTFSolver.
///
/// The call-string k-limit is fixed at 1: a selected function gets one set of
/// PAG nodes per call-site that reaches it.  Non-selected functions keep a
/// single set of nodes shared by all callers, exactly as before.
struct ContextSensitivityOptions {
  enum class Mode : uint8_t {
    Off,     ///< Root context only; identical to the insensitive solver.
    Manual,  ///< Only functions matching \c AllowList.
    Dynamic, ///< \c AllowList plus functions observed as precision-critical.
    All,     ///< Every function, until \c MaxContextualNodes is reached.
  };

  Mode SelectionMode = Mode::Off;
  /// Function-name globs (\c llvm::GlobPattern).  \c DenyList wins over
  /// \c AllowList.
  std::vector<std::string> AllowList{};
  std::vector<std::string> DenyList{};
  /// Hard cap on context-qualified PAG nodes.  Once reached, no function is
  /// newly selected and no already-selected function gets a further context:
  /// sound, just less precise.  This is the knob that bounds run time; solve
  /// time grows super-linearly in the node count, so raising it is not a
  /// proportional trade.
  size_t MaxContextualNodes = 20'000;
  /// Cap on distinct calling contexts per function; further call sites fall
  /// back to the shared root context.  A selected function costs one clone of
  /// its whole body per context, so without this a single hot function can
  /// consume \c MaxContextualNodes on its own.
  unsigned MaxContextsPerFunction = 32;
  /// \c Mode::Dynamic only: functions with more LLVM instructions than this
  /// are never selected.  Cloning a large body per context is expensive, and
  /// large functions are rarely the point where callers merge.
  unsigned MaxContextualFunctionSize = 256;
  /// \c Mode::Dynamic only: tighter size limit for the weaker signal where
  /// the merged parameters never leave the function body.  Off by default:
  /// that signal's only payoff is formal-vs-formal aliasing inside the body,
  /// which \c buildResult unions back together across contexts anyway.
  unsigned MaxLocalMergeFunctionSize = 0;

  [[nodiscard]] constexpr bool isOff() const noexcept {
    return SelectionMode == Mode::Off;
  }
};

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
/// Context-sensitivity is opt-in via \c ContextSensitivityOptions and off by
/// default.
class AndersenOTFSolver {
public:
  explicit AndersenOTFSolver(const LLVMProjectIRDB &IRDB PSR_LIFETIMEBOUND,
                             llvm::ArrayRef<const llvm::Function *> Entries
                                 PSR_LIFETIMEBOUND,
                             ValueCompressor<PAGVariable> &VC PSR_LIFETIMEBOUND,
                             Soundness S = Soundness::Soundy,
                             ContextSensitivityOptions CSOpts = {}) noexcept;

  /// Run the full OTF fixpoint and return the alias-analysis result.
  [[nodiscard]] AndersenOTFResult solve();

private:
  struct SolverData;

  NonNullPtr<const LLVMProjectIRDB> IRDB;
  llvm::ArrayRef<const llvm::Function *> Entries;
  NonNullPtr<ValueCompressor<PAGVariable>> VC;
  Soundness S;
  ContextSensitivityOptions CSOpts;
};

// ---- Factory functions ------------------------------------------------

/// Runs the Andersen OTF fixpoint and returns the raw alias-analysis result
/// (no LLVM-value wrapping).  If \p VC is null, a fresh one is allocated.
[[nodiscard]] AndersenOTFResult
computeAndersenOTFRaw(const LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<const llvm::Function *> EntryPoints,
                      MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                      Soundness S = Soundness::Soundy,
                      ContextSensitivityOptions CSOpts = {});

/// Runs the Andersen OTF fixpoint and returns an \c LLVMUnionFindAliasIterator
/// that implements \c IsLLVMAliasIterator.
[[nodiscard]] LLVMUnionFindAliasIterator<AndersenOTFResult>
computeAndersenOTF(const LLVMProjectIRDB &IRDB,
                   llvm::ArrayRef<const llvm::Function *> EntryPoints,
                   MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                   Soundness S = Soundness::Soundy,
                   ContextSensitivityOptions CSOpts = {});

} // namespace psr
