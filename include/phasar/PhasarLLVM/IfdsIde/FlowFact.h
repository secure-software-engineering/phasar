/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef FLOWFACT_H_
#define FLOWFACT_H_

#include <iostream>

class FlowFact {
 public:
  virtual ~FlowFact() = default;
  virtual std::ostream &print(std::ostream &os) const = 0;
};

#endif
