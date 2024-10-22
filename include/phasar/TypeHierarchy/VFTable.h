/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_TYPEHIERARCHY_VFTABLE_H
#define PHASAR_TYPEHIERARCHY_VFTABLE_H

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

template <typename F> class VFTable {
public:
  virtual ~VFTable() = default;

  [[nodiscard]] virtual F getFunction(unsigned Idx) const = 0;

  [[nodiscard]] virtual std::vector<F> getAllFunctions() const = 0;

  [[nodiscard]] virtual int getIndex(F Func) const = 0;

  [[nodiscard]] virtual bool empty() const = 0;

  [[nodiscard]] virtual size_t size() const = 0;

  virtual void print(llvm::raw_ostream &OS) const = 0;

  [[nodiscard,
    deprecated("Please use printAsJson() instead")]] virtual nlohmann::json
  getAsJson() const = 0;

  virtual void printAsJson(llvm::raw_ostream &OS) const = 0;
};

template <typename T, typename F>
static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                            const VFTable<F> &Table) {
  Table.print(OS);
  return OS;
}

} // namespace psr

#endif
