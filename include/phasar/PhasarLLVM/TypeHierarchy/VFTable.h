/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_VFTABLE_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_VFTABLE_H_

#include <iosfwd>
#include <vector>

#include <nlohmann/json.hpp>

namespace psr {

template <typename F> class VFTable {
public:
  virtual ~VFTable() = default;

  virtual F getFunction(unsigned Idx) const = 0;

  virtual std::vector<F> getAllFunctions() const = 0;

  virtual int getIndex(F f) const = 0;

  virtual bool empty() const = 0;

  virtual size_t size() const = 0;

  virtual void print(std::ostream &OS) const = 0;

  virtual nlohmann::json getAsJson() const = 0;
};

template <typename T, typename F>
static inline std::ostream &operator<<(std::ostream &OS,
                                       const VFTable<F> &Table) {
  Table.print(OS);
  return OS;
}

} // namespace psr

#endif
