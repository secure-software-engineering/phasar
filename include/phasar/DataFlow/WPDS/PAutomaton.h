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

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace psr {
namespace wpds {

/// Sentinel value representing the ε (empty word) stack symbol.
static constexpr uint32_t kEpsilonSym = UINT32_MAX;

/// A weighted P-automaton as defined in Definition 2 of the WPDS paper.
///
/// States are uint32_t IDs allocated by addState(). Control-location states
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
  using StateId = uint32_t;

  // ─── Transition key ────────────────────────────────────────────────────────

  struct Transition {
    StateId From;
    uint32_t Sym; ///< Stack symbol, or kEpsilonSym for ε.
    StateId To;

    [[nodiscard]] bool operator==(const Transition &O) const noexcept {
      return From == O.From && Sym == O.Sym && To == O.To;
    }
  };

  struct TransitionDSI {
    static Transition getEmptyKey() noexcept {
      return {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    }
    static Transition getTombstoneKey() noexcept {
      return {UINT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1};
    }
    static unsigned getHashValue(const Transition &T) noexcept {
      return llvm::hash_combine(T.From, T.Sym, T.To);
    }
    static bool isEqual(const Transition &A, const Transition &B) noexcept {
      return A == B;
    }
  };

  // ─── State management ──────────────────────────────────────────────────────

  /// Allocate a fresh anonymous state and return its ID.
  StateId addState() noexcept { return NumStates++; }

  /// Mark state S as an initial state (control location p ∈ P).
  void markInitial(StateId S) { InitialStates.push_back(S); }

  /// Mark state S as an accepting (final) state.
  void markFinal(StateId S) { FinalStates.push_back(S); }

  [[nodiscard]] bool isFinal(StateId S) const noexcept {
    for (StateId F : FinalStates)
      if (F == S)
        return true;
    return false;
  }

  [[nodiscard]] bool isInitial(StateId S) const noexcept {
    for (StateId I : InitialStates)
      if (I == S)
        return true;
    return false;
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
  StateId getOrCreateWitnessState(uint32_t ToLoc, uint32_t ToSym1) {
    auto Key = std::make_pair(ToLoc, ToSym1);
    auto [It, Inserted] = WitnessStates.try_emplace(Key, NumStates);
    if (Inserted)
      ++NumStates;
    return It->second;
  }

  [[nodiscard]] std::optional<StateId>
  getWitnessState(uint32_t ToLoc, uint32_t ToSym1) const noexcept {
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
  ///   - Updates ForwardIdx, OutgoingIdx, and EpsilonBackward (structural
  ///     indices are only populated when the transition is first created).
  bool update(StateId From, uint32_t Sym, StateId To, const Weight &V) {
    Transition T{From, Sym, To};
    auto [It, Inserted] = Weights.try_emplace(T, Weight::zero());
    if (Inserted) {
      // Register in structural indices (done once per transition).
      ForwardIdx[{From, Sym}].push_back(To);
      OutgoingIdx[From].push_back({Sym, To});
      if (Sym == kEpsilonSym)
        EpsilonBackward[To].push_back(From);
    }
    Weight NewVal = It->second.combine(V);
    if (NewVal == It->second)
      return false;
    It->second = std::move(NewVal);
    return true;
  }

  /// Returns the weight of transition (From, Sym, To), or zero() if absent.
  [[nodiscard]] Weight getWeight(StateId From, uint32_t Sym,
                                 StateId To) const noexcept {
    auto It = Weights.find({From, Sym, To});
    return (It != Weights.end()) ? It->second : Weight::zero();
  }

  [[nodiscard]] bool hasTransition(StateId From, uint32_t Sym,
                                   StateId To) const noexcept {
    return Weights.count({From, Sym, To}) != 0;
  }

  // ─── Structural queries ────────────────────────────────────────────────────

  /// Returns all To states reachable from (From, Sym), together with their
  /// weights. Used in Algorithm 3 line 25.
  [[nodiscard]] llvm::ArrayRef<StateId>
  getSuccessors(StateId From, uint32_t Sym) const noexcept {
    auto It = ForwardIdx.find({From, Sym});
    if (It == ForwardIdx.end())
      return {};
    return It->second;
  }

  /// Returns all (Sym, To) pairs for transitions leaving state From.
  /// Used in Algorithm 3 line 25 (ε-closure propagation).
  [[nodiscard]] llvm::ArrayRef<std::pair<uint32_t, StateId>>
  getOutgoing(StateId From) const noexcept {
    auto It = OutgoingIdx.find(From);
    if (It == OutgoingIdx.end())
      return {};
    return It->second;
  }

  /// Returns all From states that have an ε-transition to To.
  /// Used in Algorithm 3 line 23 (push-rule propagation through ε-trans).
  [[nodiscard]] llvm::ArrayRef<StateId>
  getEpsilonPredecessors(StateId To) const noexcept {
    auto It = EpsilonBackward.find(To);
    if (It == EpsilonBackward.end())
      return {};
    return It->second;
  }

  /// Access the raw transition weight map (for result inspection / Algorithm
  /// 4).
  [[nodiscard]] const llvm::DenseMap<Transition, Weight, TransitionDSI> &
  getWeights() const noexcept {
    return Weights;
  }

private:
  struct PairDSI {
    using P = std::pair<uint32_t, uint32_t>;
    static P getEmptyKey() noexcept { return {UINT32_MAX, UINT32_MAX}; }
    static P getTombstoneKey() noexcept {
      return {UINT32_MAX - 1, UINT32_MAX - 1};
    }
    static unsigned getHashValue(P V) noexcept {
      return llvm::hash_combine(V.first, V.second);
    }
    static bool isEqual(P A, P B) noexcept { return A == B; }
  };

  uint32_t NumStates = 0;

  llvm::SmallVector<StateId, 4> InitialStates;
  llvm::SmallVector<StateId, 4> FinalStates;

  /// Witness states: (ToLoc, ToSym1) → StateId.
  llvm::DenseMap<std::pair<uint32_t, uint32_t>, StateId, PairDSI> WitnessStates;

  /// Transition weights (the authoritative store).
  llvm::DenseMap<Transition, Weight, TransitionDSI> Weights;

  /// Forward structural index: (From, Sym) → list of To states.
  llvm::DenseMap<std::pair<StateId, uint32_t>, llvm::SmallVector<StateId, 4>,
                 PairDSI>
      ForwardIdx;

  /// Outgoing index: From → list of (Sym, To) pairs.
  llvm::DenseMap<StateId, llvm::SmallVector<std::pair<uint32_t, StateId>, 8>>
      OutgoingIdx;

  /// Epsilon-backward index: To → list of From states (for ε-transitions).
  llvm::DenseMap<StateId, llvm::SmallVector<StateId, 4>> EpsilonBackward;
};

} // namespace wpds
} // namespace psr
