/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <ostream>

#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>

namespace psr {

template <typename T> class FlowFactWrapper : public FlowFact {
private:
  T fact;

public:
  FlowFactWrapper(T f) : fact(f) {}
  virtual ~FlowFactWrapper() = default;
  T get() { return fact; }
  std::ostream &print(std::ostream &os) const override {
    return os << fact << '\n';
  }
};
} // namespace psr
