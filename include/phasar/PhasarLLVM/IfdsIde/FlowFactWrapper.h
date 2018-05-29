/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef FLOWFACTWRAPPER_H_
#define FLOWFACTWRAPPER_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>

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

#endif
