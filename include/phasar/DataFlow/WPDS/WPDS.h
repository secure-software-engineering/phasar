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
#include "phasar/DataFlow/WPDS/WPDSRule.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
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
  void addPopRule(LocId FromLoc, SymId FromSym, LocId ToLoc, Weight Wt) {
    appendRule({
        WPDSRuleKind::Pop,
        FromLoc,
        FromSym,
        ToLoc,
        {},
        {},
        std::move(Wt),
    });
  }

  /// Add an internal (swap) rule: (FromLoc, FromSym) ↪ (ToLoc, ToSym).
  void addInternalRule(LocId FromLoc, SymId FromSym, LocId ToLoc, SymId ToSym,
                       Weight Wt) {
    appendRule({
        WPDSRuleKind::Internal,
        FromLoc,
        FromSym,
        ToLoc,
        ToSym,
        {},
        std::move(Wt),
    });
  }

  /// Add a push rule: (FromLoc, FromSym) ↪ (ToLoc, ToSym1 ToSym2).
  void addPushRule(LocId FromLoc, SymId FromSym, LocId ToLoc, SymId ToSym1,
                   SymId ToSym2, Weight Wt) {
    appendRule({WPDSRuleKind::Push, FromLoc, FromSym, ToLoc, ToSym1, ToSym2,
                std::move(Wt)});
  }

  /// Returns the indices of all rules with the given (FromLoc, FromSym) pair.
  [[nodiscard]] llvm::ArrayRef<uint32_t>
  getRulesFor(LocId FromLoc, SymId FromSym) const noexcept {
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

  llvm::SmallVector<Rule> Rules;
  // DenseMapInfo<pair<LocId, SymId>> is auto-derived from the individual
  // DenseMapInfo specialisations produced by PHASAR_STRONG_TYPEDEF.
  llvm::DenseMap<std::pair<LocId, SymId>, llvm::SmallVector<uint32_t, 4>>
      RuleIndex;
};

} // namespace wpds
} // namespace psr
