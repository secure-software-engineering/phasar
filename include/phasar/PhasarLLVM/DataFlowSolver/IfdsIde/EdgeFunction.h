/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractEdgeFunction.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTION_H_

#include <memory>
#include <ostream>
#include <sstream>
#include <string>

namespace psr {

template <typename L> class EdgeFunction {
public:
  virtual ~EdgeFunction() = default;

  virtual L computeTarget(L source) = 0;

  virtual EdgeFunction<L> *composeWith(EdgeFunction<L> *secondFunction) = 0;

  virtual EdgeFunction<L> *joinWith(EdgeFunction<L> *otherFunction) = 0;

  virtual bool equal_to(EdgeFunction<L> *other) const = 0;

  virtual void print(std::ostream &OS, bool isForDebug = false) const {
    OS << "EdgeFunction";
  }

  std::string str() {
    std::ostringstream oss;
    print(oss);
    return oss.str();
  }
};

template <typename L>
static inline bool operator==(const EdgeFunction<L> &F,
                              const EdgeFunction<L> &G) {
  return F.equal_to(G);
}

template <typename L>
static inline std::ostream &operator<<(std::ostream &OS,
                                       const EdgeFunction<L> &F) {
  F.print(OS);
  return OS;
}

} // namespace psr

#endif
