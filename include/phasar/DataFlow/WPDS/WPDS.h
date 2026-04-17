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
#include "phasar/DataFlow/WPDS/WPDSRule.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <utility>

namespace psr {
namespace wpds {

/// Stores all rules of a Weighted Pushdown System, indexed by (FromLoc,
/// FromSym) for O(1) lookup during the post* saturation algorithm.
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight>
  requires BoundedIdempotentSemiring<Weight>
class WeightedPushdownSystem {
public:
  using Rule = WPDSRule<Weight>;

  /// Add a pop rule: (FromLoc, FromSym) ↪ (ToLoc, ε).
  void addPopRule(uint32_t FromLoc, uint32_t FromSym, uint32_t ToLoc,
                  Weight Wt) {
    appendRule(
        {WPDSRuleKind::Pop, FromLoc, FromSym, ToLoc, 0, 0, std::move(Wt)});
  }

  /// Add an internal (swap) rule: (FromLoc, FromSym) ↪ (ToLoc, ToSym).
  void addInternalRule(uint32_t FromLoc, uint32_t FromSym, uint32_t ToLoc,
                       uint32_t ToSym, Weight Wt) {
    appendRule({WPDSRuleKind::Internal, FromLoc, FromSym, ToLoc, ToSym, 0,
                std::move(Wt)});
  }

  /// Add a push rule: (FromLoc, FromSym) ↪ (ToLoc, ToSym1 ToSym2).
  void addPushRule(uint32_t FromLoc, uint32_t FromSym, uint32_t ToLoc,
                   uint32_t ToSym1, uint32_t ToSym2, Weight Wt) {
    appendRule({WPDSRuleKind::Push, FromLoc, FromSym, ToLoc, ToSym1, ToSym2,
                std::move(Wt)});
  }

  /// Returns the indices of all rules with the given (FromLoc, FromSym) pair.
  [[nodiscard]] llvm::ArrayRef<uint32_t>
  getRulesFor(uint32_t FromLoc, uint32_t FromSym) const noexcept {
    auto It = RuleIndex.find({FromLoc, FromSym});
    if (It == RuleIndex.end())
      return {};
    return It->second;
  }

  /// Returns the rule at the given index.
  [[nodiscard]] const Rule &getRule(uint32_t Idx) const noexcept {
    return Rules[Idx];
  }

  /// Returns all rules in insertion order.
  [[nodiscard]] llvm::ArrayRef<Rule> getAllRules() const noexcept {
    return Rules;
  }

  [[nodiscard]] size_t getNumRules() const noexcept { return Rules.size(); }

private:
  void appendRule(Rule R) {
    auto Idx = static_cast<uint32_t>(Rules.size());
    RuleIndex[{R.FromLoc, R.FromSym}].push_back(Idx);
    Rules.push_back(std::move(R));
  }

  struct PairDenseMapInfo {
    using Pair = std::pair<uint32_t, uint32_t>;
    static Pair getEmptyKey() noexcept { return {UINT32_MAX, UINT32_MAX}; }
    static Pair getTombstoneKey() noexcept {
      return {UINT32_MAX - 1, UINT32_MAX - 1};
    }
    static unsigned getHashValue(Pair P) noexcept {
      return llvm::hash_combine(P.first, P.second);
    }
    static bool isEqual(Pair A, Pair B) noexcept { return A == B; }
  };

  llvm::SmallVector<Rule> Rules;
  llvm::DenseMap<std::pair<uint32_t, uint32_t>, llvm::SmallVector<uint32_t, 4>,
                 PairDenseMapInfo>
      RuleIndex;
};

} // namespace wpds
} // namespace psr
