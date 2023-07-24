/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUESET_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUESET_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/Utils/JoinLattice.h"

#include <initializer_list>
#include <unordered_set>

namespace psr::glca {

class EdgeValueSet {
  std::unordered_set<EdgeValue> Underlying;

public:
  EdgeValueSet();
  template <typename Iter>
  EdgeValueSet(Iter Begin, Iter End) : Underlying(Begin, End) {}
  EdgeValueSet(std::initializer_list<EdgeValue> IList);
  auto begin() -> decltype(Underlying.begin());
  auto end() -> decltype(Underlying.end());
  auto begin() const -> decltype(Underlying.begin());
  auto end() const -> decltype(Underlying.end());
  int count(const EdgeValue &EV) const;
  auto find(const EdgeValue &EV) -> decltype(Underlying.find(EV));
  auto find(const EdgeValue &EV) const -> decltype(Underlying.find(EV));

  size_t size() const;
  auto insert(const EdgeValue &EV) -> decltype(Underlying.insert(EV));
  auto insert(EdgeValue &&EV) -> decltype(Underlying.insert(EV));
  bool empty() const;
  bool operator==(const EdgeValueSet &Other) const;
  bool operator!=(const EdgeValueSet &Other) const;
};

} // namespace psr::glca

namespace psr {
template <> struct JoinLatticeTraits<glca::EdgeValueSet> {
  using l_t = glca::EdgeValueSet;

  static l_t bottom() { return l_t({glca::EdgeValue::TopValue}); }
  static l_t top() { return l_t({}); }
  static l_t join(const l_t &LHS, const l_t &RHS) {
    return glca::join(LHS, RHS, 2);
  }
};
} // namespace psr

#endif
