/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/DataFlow/WPDS/Semiring.h"
#include "phasar/DataFlow/WPDS/WPDSIds.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace psr {
namespace wpds {

/// A weighted P-automaton as defined in Definition 2 of the WPDS paper.
///
/// States are StateId values allocated by addState(). Control-location states
/// (P) must be explicitly registered via markInitial(). Accepting states are
/// registered via markFinal().
///
/// Transitions carry semiring weights. The special symbol kEpsilonSym
/// represents ε-transitions, which appear in the output of post* when pop
/// rules are applied.
///
/// The automaton supports three kinds of structural queries needed by
/// Algorithm 3 (Fig. 17):
///   1. Forward: (From, Sym) → list of To states + weight.
///   2. Outgoing: From → all (Sym, To, Weight) triples.
///   3. Epsilon-backward: To → list of From states for ε-transitions.
///      Used in the "if changed" branch of the push-rule case (line 23).
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight>
  requires BoundedIdempotentSemiring<Weight>
class PAutomaton {
public:
  // ─── Transition key ────────────────────────────────────────────────────────

  struct Transition {
    StateId From;
    SymId Sym; ///< Stack symbol, or kEpsilonSym for ε.
    StateId To;

    [[nodiscard]] bool operator==(const Transition &O) const noexcept {
      return From == O.From && Sym == O.Sym && To == O.To;
    }
  };

  struct TransitionDSI {
    static Transition getEmptyKey() noexcept {
      return {StateId{~0U}, SymId{~0U}, StateId{~0U}};
    }
    static Transition getTombstoneKey() noexcept {
      return {StateId{~0U - 1}, SymId{~0U - 1}, StateId{~0U - 1}};
    }
    static unsigned getHashValue(const Transition &T) noexcept {
      return llvm::hash_combine(to_underlying(T.From), to_underlying(T.Sym),
                                to_underlying(T.To));
    }
    static bool isEqual(const Transition &A, const Transition &B) noexcept {
      return A == B;
    }
  };

  // ─── State management ──────────────────────────────────────────────────────

  /// Allocate a fresh anonymous state and return its ID.
  StateId addState() {
    StateId S{NumStates++};
    OutgoingVec.emplace_back();
    EpsilonBackwardVec.emplace_back();
    return S;
  }

  /// Mark state S as an initial state (control location p ∈ P).
  void markInitial(StateId S) {
    InitialStates.push_back(S);
    ensureCapacity(S);
    IsInitial.insert(S);
  }

  /// Mark state S as an accepting (final) state.
  void markFinal(StateId S) {
    FinalStates.push_back(S);
    ensureCapacity(S);
    IsFinal.insert(S);
  }

  [[nodiscard]] bool isFinal(StateId S) const noexcept {
    return IsFinal.contains(S);
  }

  [[nodiscard]] bool isInitial(StateId S) const noexcept {
    return IsInitial.contains(S);
  }

  [[nodiscard]] uint32_t getNumStates() const noexcept { return NumStates; }
  [[nodiscard]] llvm::ArrayRef<StateId> getFinalStates() const noexcept {
    return FinalStates;
  }
  [[nodiscard]] llvm::ArrayRef<StateId> getInitialStates() const noexcept {
    return InitialStates;
  }

  // ─── Witness state management ──────────────────────────────────────────────

  /// Get (or create) the witness state q_{ToLoc, ToSym1} for a push rule
  /// (p, γ) ↪ (ToLoc, ToSym1 ToSym2). One witness state is shared across all
  /// push rules with the same (ToLoc, ToSym1) pair.
  StateId getOrCreateWitnessState(LocId ToLoc, SymId ToSym1) {
    auto Key = std::make_pair(ToLoc, ToSym1);
    auto [It, Inserted] = WitnessStates.try_emplace(Key, StateId{NumStates});
    if (Inserted)
      addState();
    return It->second;
  }

  [[nodiscard]] std::optional<StateId>
  getWitnessState(LocId ToLoc, SymId ToSym1) const noexcept {
    auto It = WitnessStates.find({ToLoc, ToSym1});
    if (It == WitnessStates.end())
      return std::nullopt;
    return It->second;
  }

  // ─── Transition management ─────────────────────────────────────────────────

  /// Update the weight of transition (From, Sym, To) by combining the current
  /// weight with V using the semiring combine (⊕) operation.
  ///
  /// Returns true iff the weight improved, i.e., the weight field changed.
  /// On the first call for a given (From, Sym, To) triple, the transition is
  /// created with initial weight zero(), so any non-zero V triggers a change.
  ///
  /// Side effects on change:
  ///   - Updates ForwardIdx, OutgoingVec, and EpsilonBackwardVec (structural
  ///     indices are only populated when the transition is first created).
  bool update(StateId From, SymId Sym, StateId To, const Weight &V) {
    Transition T{From, Sym, To};
    auto [It, Inserted] = Weights.try_emplace(T, Weight::zero());
    if (Inserted) {
      ensureCapacity(From);
      ensureCapacity(To);
      // Register in structural indices (done once per transition).
      ForwardIdx[{From, Sym}].push_back(To);
      OutgoingVec[From].push_back({Sym, To});
      if (Sym == kEpsilonSym)
        EpsilonBackwardVec[To].push_back(From);
    }
    Weight NewVal = It->second.combine(V);
    if (NewVal == It->second)
      return false;
    It->second = std::move(NewVal);
    return true;
  }

  /// Returns the weight of transition (From, Sym, To), or zero() if absent.
  [[nodiscard]] Weight getWeight(StateId From, SymId Sym,
                                 StateId To) const noexcept {
    auto It = Weights.find({From, Sym, To});
    return (It != Weights.end()) ? It->second : Weight::zero();
  }

  [[nodiscard]] bool hasTransition(StateId From, SymId Sym,
                                   StateId To) const noexcept {
    return Weights.count({From, Sym, To}) != 0;
  }

  // ─── Structural queries ────────────────────────────────────────────────────

  /// Returns all To states reachable from (From, Sym), together with their
  /// weights. Used in Algorithm 3 line 25.
  [[nodiscard]] llvm::ArrayRef<StateId>
  getSuccessors(StateId From, SymId Sym) const noexcept {
    auto It = ForwardIdx.find({From, Sym});
    if (It == ForwardIdx.end())
      return {};
    return It->second;
  }

  /// Returns all (Sym, To) pairs for transitions leaving state From.
  /// Used in Algorithm 3 line 25 (ε-closure propagation).
  [[nodiscard]] llvm::ArrayRef<std::pair<SymId, StateId>>
  getOutgoing(StateId From) const noexcept {
    if (to_underlying(From) >= OutgoingVec.size())
      return {};
    return OutgoingVec[From];
  }

  /// Returns all From states that have an ε-transition to To.
  /// Used in Algorithm 3 line 23 (push-rule propagation through ε-trans).
  [[nodiscard]] llvm::ArrayRef<StateId>
  getEpsilonPredecessors(StateId To) const noexcept {
    if (to_underlying(To) >= EpsilonBackwardVec.size())
      return {};
    return EpsilonBackwardVec[To];
  }

  /// Access the raw transition weight map (for result inspection / Algorithm
  /// 4).
  [[nodiscard]] const llvm::DenseMap<Transition, Weight, TransitionDSI> &
  getWeights() const noexcept {
    return Weights;
  }

private:
  void ensureCapacity(StateId S) {
    auto Idx = to_underlying(S);
    if (Idx >= OutgoingVec.size()) {
      OutgoingVec.resize(Idx + 1);
      EpsilonBackwardVec.resize(Idx + 1);
    }
  }

  uint32_t NumStates = 0;

  llvm::SmallVector<StateId, 4> InitialStates;
  llvm::SmallVector<StateId, 4> FinalStates;

  /// O(1) membership tests for initial/final states.
  BitSet<StateId> IsInitial;
  BitSet<StateId> IsFinal;

  /// Witness states: (ToLoc, ToSym1) → StateId.
  // DenseMapInfo<pair<LocId, SymId>> is auto-derived via PHASAR_STRONG_TYPEDEF.
  llvm::DenseMap<std::pair<LocId, SymId>, StateId> WitnessStates;

  /// Transition weights (the authoritative store).
  llvm::DenseMap<Transition, Weight, TransitionDSI> Weights;

  /// Forward structural index: (From, Sym) → list of To states.
  // DenseMapInfo<pair<StateId, SymId>> is auto-derived.
  llvm::DenseMap<std::pair<StateId, SymId>, llvm::SmallVector<StateId, 4>>
      ForwardIdx;

  /// Outgoing index: From → list of (Sym, To) pairs. Direct-indexed by StateId.
  TypedVector<StateId, llvm::SmallVector<std::pair<SymId, StateId>, 8>>
      OutgoingVec;

  /// Epsilon-backward index: To → list of From states. Direct-indexed by
  /// StateId.
  TypedVector<StateId, llvm::SmallVector<StateId, 4>> EpsilonBackwardVec;
};

} // namespace wpds
} // namespace psr
