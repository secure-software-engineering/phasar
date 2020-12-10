/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_TYPEHIERARCHY_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_TYPEHIERARCHY_H_

#include <iostream>
#include <set>
#include <string>

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/TypeHierarchy/VFTable.h"

namespace psr {

template <typename T, typename F> class TypeHierarchy {
public:
  virtual ~TypeHierarchy() = default;

  virtual bool hasType(T Type) const = 0;

  virtual bool isSubType(T Type, T SubType) = 0;

  virtual std::set<T> getSubTypes(T Type) = 0;

  virtual bool isSuperType(T Type, T SuperType) = 0;

  virtual std::set<T> getSuperTypes(T Type) = 0;

  virtual T getType(std::string TypeName) const = 0;

  virtual std::set<T> getAllTypes() const = 0;

  virtual std::string getTypeName(T Type) const = 0;

  virtual bool hasVFTable(T Type) const = 0;

  virtual const VFTable<F> *getVFTable(T Type) const = 0;

  virtual size_t size() const = 0;

  virtual bool empty() const = 0;

  virtual void print(std::ostream &OS = std::cout) const = 0;

  virtual nlohmann::json getAsJson() const = 0;
};

template <typename T, typename F>
static inline std::ostream &operator<<(std::ostream &OS,
                                       const TypeHierarchy<T, F> &TH) {
  TH.print(OS);
  return OS;
}

} // namespace psr

#endif
