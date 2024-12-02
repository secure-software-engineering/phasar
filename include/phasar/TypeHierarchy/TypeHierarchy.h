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

#include "phasar/Utils/Nullable.h"

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json_fwd.hpp"

#include <set>

namespace psr {

template <typename T, typename F> class TypeHierarchy {
public:
  virtual ~TypeHierarchy() = default;

  [[nodiscard]] virtual bool hasType(T Type) const = 0;
  [[nodiscard]] virtual bool isSubType(T Type, T SubType) const = 0;

  [[nodiscard]] virtual std::set<T> getSubTypes(T Type) const = 0;

  [[nodiscard]] virtual Nullable<T> getType(llvm::StringRef TypeName) const = 0;

  [[nodiscard]] virtual std::vector<T> getAllTypes() const = 0;

  [[nodiscard]] virtual llvm::StringRef getTypeName(T Type) const = 0;

  [[nodiscard]] virtual size_t size() const noexcept = 0;
  [[nodiscard]] virtual bool empty() const noexcept = 0;

  virtual void print(llvm::raw_ostream &OS = llvm::outs()) const = 0;

  [[nodiscard,
    deprecated("Please use printAsJson() instead")]] virtual nlohmann::json
  getAsJson() const = 0;

  virtual void printAsJson(llvm::raw_ostream &OS) const = 0;
};

template <typename T, typename F>
static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                            const TypeHierarchy<T, F> &TH) {
  TH.print(OS);
  return OS;
}

} // namespace psr

#endif
