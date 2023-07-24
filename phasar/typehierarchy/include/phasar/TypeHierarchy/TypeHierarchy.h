/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_TYPEHIERARCHY_TYPEHIERARCHY_H
#define PHASAR_TYPEHIERARCHY_TYPEHIERARCHY_H

#include "phasar/TypeHierarchy/VFTable.h"

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <set>
#include <string>

namespace psr {

template <typename T, typename F> class TypeHierarchy {
public:
  virtual ~TypeHierarchy() = default;

  virtual bool hasType(T Type) const = 0;

  virtual bool isSubType(T Type, T SubType) = 0;

  virtual std::set<T> getSubTypes(T Type) = 0;

  virtual bool isSuperType(T Type, T SuperType) = 0;

  virtual std::set<T> getSuperTypes(T Type) = 0;

  [[nodiscard]] virtual T getType(std::string TypeName) const = 0;

  [[nodiscard]] virtual std::set<T> getAllTypes() const = 0;

  [[nodiscard]] virtual std::string getTypeName(T Type) const = 0;

  [[nodiscard]] virtual bool hasVFTable(T Type) const = 0;

  [[nodiscard]] virtual const VFTable<F> *getVFTable(T Type) const = 0;

  [[nodiscard]] virtual size_t size() const = 0;

  [[nodiscard]] virtual bool empty() const = 0;

  virtual void print(llvm::raw_ostream &OS = llvm::outs()) const = 0;

  [[nodiscard]] virtual nlohmann::json getAsJson() const = 0;
};

template <typename T, typename F>
static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                            const TypeHierarchy<T, F> &TH) {
  TH.print(OS);
  return OS;
}

} // namespace psr

#endif
