/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUESET_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_EDGEVALUESET_H_

#include <initializer_list>
#include <unordered_set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"

namespace psr {

class EdgeValueSet {
  std::unordered_set<EdgeValue> underlying;

public:
  EdgeValueSet();
  template <typename Iter>
  EdgeValueSet(Iter beg, Iter ed) : underlying(beg, ed) {}
  EdgeValueSet(std::initializer_list<EdgeValue> ilist);
  auto begin() -> decltype(underlying.begin());
  auto end() -> decltype(underlying.end());
  auto begin() const -> decltype(underlying.begin());
  auto end() const -> decltype(underlying.end());
  int count(const EdgeValue &ev) const;
  auto find(const EdgeValue &ev) -> decltype(underlying.find(ev));
  auto find(const EdgeValue &ev) const -> decltype(underlying.find(ev));

  size_t size() const;
  auto insert(const EdgeValue &ev) -> decltype(underlying.insert(ev));
  auto insert(EdgeValue &&ev) -> decltype(underlying.insert(ev));
  bool empty() const;
  bool operator==(const EdgeValueSet &other) const;
  bool operator!=(const EdgeValueSet &other) const;
};

} // namespace psr

#endif
