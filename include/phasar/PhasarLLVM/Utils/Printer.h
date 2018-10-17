/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Printer.h
 *
 *  Created on: 07.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_PHASARLLVM_UTILS_PRINTER_H_
#define PHASAR_PHASARLLVM_UTILS_PRINTER_H_

#include <ostream>
#include <sstream>
#include <string>

namespace psr {

template <typename N> struct NodePrinter {
  virtual void printNode(std::ostream &os, N n) const = 0;

  virtual std::string NtoString(N n) const {
    std::stringstream ss;
    printNode(ss, n);
    return ss.str();
  }
};

template <typename D> struct DataFlowFactPrinter {
  virtual void printDataFlowFact(std::ostream &os, D d) const = 0;

  virtual std::string DtoString(D d) const {
    std::stringstream ss;
    printDataFlowFact(ss, d);
    return ss.str();
  }
};

template <typename V> struct ValuePrinter {
  virtual void printValue(std::ostream &os, V v) const = 0;

  virtual std::string VtoString(V v) const {
    std::stringstream ss;
    printValue(ss, v);
    return ss.str();
  }
};

template <typename M> struct MethodPrinter {
  virtual void printMethod(std::ostream &os, M m) const = 0;

  virtual std::string MtoString(M m) const {
    std::stringstream ss;
    printMethod(ss, m);
    return ss.str();
  }
};

} // namespace psr

#endif
