/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"

namespace psr {

EdgeValueSet::EdgeValueSet() : underlying({EdgeValue(nullptr)}) {}

EdgeValueSet::EdgeValueSet(std::initializer_list<EdgeValue> ilist)
    : underlying(ilist) {}
auto EdgeValueSet::begin() -> decltype(underlying.begin()) {
  return underlying.begin();
}
auto EdgeValueSet::end() -> decltype(underlying.end()) {
  return underlying.end();
}
auto EdgeValueSet::begin() const -> decltype(underlying.begin()) {
  return underlying.begin();
}
auto EdgeValueSet::end() const -> decltype(underlying.end()) {
  return underlying.end();
}
int EdgeValueSet::count(const EdgeValue &ev) const {
  return underlying.count(ev);
}
auto EdgeValueSet::find(const EdgeValue &ev) -> decltype(underlying.find(ev)) {
  return underlying.find(ev);
}
auto EdgeValueSet::find(const EdgeValue &ev) const
    -> decltype(underlying.find(ev)) {
  return underlying.find(ev);
}

size_t EdgeValueSet::size() const { return underlying.size(); }
auto EdgeValueSet::insert(const EdgeValue &ev)
    -> decltype(underlying.insert(ev)) {
  return underlying.insert(ev);
}
auto EdgeValueSet::insert(EdgeValue &&ev) -> decltype(underlying.insert(ev)) {
  return underlying.insert(ev);
}
bool EdgeValueSet::empty() const { return underlying.empty(); }
bool EdgeValueSet::operator==(const EdgeValueSet &other) const {
  return underlying == other.underlying;
}
bool EdgeValueSet::operator!=(const EdgeValueSet &other) const {
  return underlying != other.underlying;
}

} // namespace psr
