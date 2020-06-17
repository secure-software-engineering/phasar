/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFACT_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFACT_H_

#include <iosfwd>
#include <type_traits>

namespace psr {

class FlowFact {
public:
  virtual ~FlowFact() = default;
  virtual void print(std::ostream &OS) const = 0;

  template <typename T> T *as() {
    static_assert(std::is_base_of<FlowFact, T>::value);
    return reinterpret_cast<T *>(this);
  }
  template <typename T> const T *as() const {
    static_assert(std::is_base_of<FlowFact, T>::value);
    return reinterpret_cast<const T *>(this);
  }
};

static inline std::ostream &operator<<(std::ostream &OS, const FlowFact &F) {
  F.print(OS);
  return OS;
}

} // namespace psr

#endif
