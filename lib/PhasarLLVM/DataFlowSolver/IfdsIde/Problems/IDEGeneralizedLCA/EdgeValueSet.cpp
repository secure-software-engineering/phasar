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

EdgeValueSet::EdgeValueSet(std::initializer_list<EdgeValue> Ilist)
    : underlying(Ilist) {}
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
int EdgeValueSet::count(const EdgeValue &Ev) const {
  return underlying.count(Ev);
}
auto EdgeValueSet::find(const EdgeValue &Ev) -> decltype(underlying.find(Ev)) {
  return underlying.find(Ev);
}
auto EdgeValueSet::find(const EdgeValue &Ev) const
    -> decltype(underlying.find(Ev)) {
  return underlying.find(Ev);
}

size_t EdgeValueSet::size() const { return underlying.size(); }
auto EdgeValueSet::insert(const EdgeValue &Ev)
    -> decltype(underlying.insert(Ev)) {
  return underlying.insert(Ev);
}
auto EdgeValueSet::insert(EdgeValue &&Ev) -> decltype(underlying.insert(Ev)) {
  return underlying.insert(Ev);
}
bool EdgeValueSet::empty() const { return underlying.empty(); }
bool EdgeValueSet::operator==(const EdgeValueSet &Other) const {
  return underlying == Other.underlying;
}
bool EdgeValueSet::operator!=(const EdgeValueSet &Other) const {
  return underlying != Other.underlying;
}

} // namespace psr
