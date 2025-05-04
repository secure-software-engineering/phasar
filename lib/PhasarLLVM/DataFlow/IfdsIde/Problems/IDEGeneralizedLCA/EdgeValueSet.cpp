/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"

namespace psr::glca {

EdgeValueSet::EdgeValueSet() : Underlying({EdgeValue(nullptr)}) {}

EdgeValueSet::EdgeValueSet(std::initializer_list<EdgeValue> Ilist)
    : Underlying(Ilist) {}
auto EdgeValueSet::begin() -> decltype(Underlying.begin()) {
  return Underlying.begin();
}
auto EdgeValueSet::end() -> decltype(Underlying.end()) {
  return Underlying.end();
}
auto EdgeValueSet::begin() const -> decltype(Underlying.begin()) {
  return Underlying.begin();
}
auto EdgeValueSet::end() const -> decltype(Underlying.end()) {
  return Underlying.end();
}
int EdgeValueSet::count(const EdgeValue &Ev) const {
  return Underlying.count(Ev);
}
auto EdgeValueSet::find(const EdgeValue &Ev) -> decltype(Underlying.find(Ev)) {
  return Underlying.find(Ev);
}
auto EdgeValueSet::find(const EdgeValue &Ev) const
    -> decltype(Underlying.find(Ev)) {
  return Underlying.find(Ev);
}

size_t EdgeValueSet::size() const { return Underlying.size(); }
auto EdgeValueSet::insert(const EdgeValue &Ev)
    -> decltype(Underlying.insert(Ev)) {
  return Underlying.insert(Ev);
}
auto EdgeValueSet::insert(EdgeValue &&Ev) -> decltype(Underlying.insert(Ev)) {
  return Underlying.insert(Ev);
}
bool EdgeValueSet::empty() const { return Underlying.empty(); }
bool EdgeValueSet::operator==(const EdgeValueSet &Other) const {
  return Underlying == Other.Underlying;
}
bool EdgeValueSet::operator!=(const EdgeValueSet &Other) const {
  return Underlying != Other.Underlying;
}

} // namespace psr::glca
