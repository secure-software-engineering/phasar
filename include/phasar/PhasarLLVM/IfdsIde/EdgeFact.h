/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef EDGEFACT_H_
#define EDGEFACT_H_

#include <iostream>

class EdgeFact {
public:
  virtual ~EdgeFact() = default;
  virtual std::ostream &print(std::ostream &os) const = 0;
};

#endif
