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

template <typename AnalysisDomainTy> struct NodePrinter {
  using N = typename AnalysisDomainTy::n_t;

  NodePrinter() = default;
  NodePrinter(const NodePrinter &) = delete;
  NodePrinter &operator=(const NodePrinter &) = delete;
  NodePrinter(NodePrinter &&) = delete;
  NodePrinter &operator=(NodePrinter &&) = delete;
  virtual ~NodePrinter() = default;

  virtual void printNode(std::ostream &os, N n) const = 0;

  virtual std::string NtoString(N n) const {
    std::stringstream ss;
    printNode(ss, n);
    return ss.str();
  }
};

template <typename AnalysisDomainTy> struct DataFlowFactPrinter {
  using D = typename AnalysisDomainTy::d_t;

  DataFlowFactPrinter() = default;
  DataFlowFactPrinter(const DataFlowFactPrinter &) = delete;
  DataFlowFactPrinter &operator=(const DataFlowFactPrinter &) = delete;
  DataFlowFactPrinter(DataFlowFactPrinter &&) = delete;
  DataFlowFactPrinter &operator=(DataFlowFactPrinter &&) = delete;
  virtual ~DataFlowFactPrinter() = default;

  virtual void printDataFlowFact(std::ostream &os, D d) const = 0;

  virtual std::string DtoString(D d) const {
    std::stringstream ss;
    printDataFlowFact(ss, d);
    return ss.str();
  }
};

template <typename V> struct ValuePrinter {
  ValuePrinter() = default;
  ValuePrinter(const ValuePrinter &) = delete;
  ValuePrinter &operator=(const ValuePrinter &) = delete;
  ValuePrinter(ValuePrinter &&) = delete;
  ValuePrinter &operator=(ValuePrinter &&) = delete;
  virtual ~ValuePrinter() = default;

  virtual void printValue(std::ostream &os, V v) const = 0;

  virtual std::string VtoString(V v) const {
    std::stringstream ss;
    printValue(ss, v);
    return ss.str();
  }
};

template <typename T> struct TypePrinter {
  TypePrinter() = default;
  TypePrinter(const TypePrinter &) = delete;
  TypePrinter &operator=(const TypePrinter &) = delete;
  TypePrinter(TypePrinter &&) = delete;
  TypePrinter &operator=(TypePrinter &&) = delete;
  virtual ~TypePrinter() = default;

  virtual void printType(std::ostream &os, T t) const = 0;

  virtual std::string TtoString(T t) const {
    std::stringstream ss;
    printType(ss, t);
    return ss.str();
  }
};

template <typename AnalysisDomainTy> struct EdgeFactPrinter {
  using l_t = typename AnalysisDomainTy::l_t;

  EdgeFactPrinter() = default;
  EdgeFactPrinter(const EdgeFactPrinter &) = delete;
  EdgeFactPrinter &operator=(const EdgeFactPrinter &) = delete;
  EdgeFactPrinter(EdgeFactPrinter &&) = delete;
  EdgeFactPrinter &operator=(EdgeFactPrinter &&) = delete;
  virtual ~EdgeFactPrinter() = default;

  virtual void printEdgeFact(std::ostream &os, l_t l) const = 0;

  [[nodiscard]] virtual std::string LtoString(l_t l) const {
    std::stringstream ss;
    printEdgeFact(ss, l);
    return ss.str();
  }
};

template <typename AnalysisDomainTy> struct FunctionPrinter {
  using F = typename AnalysisDomainTy::f_t;

  FunctionPrinter() = default;
  FunctionPrinter(const FunctionPrinter &) = delete;
  FunctionPrinter &operator=(const FunctionPrinter &) = delete;
  FunctionPrinter(FunctionPrinter &&) = delete;
  FunctionPrinter &operator=(FunctionPrinter &&) = delete;
  virtual ~FunctionPrinter() = default;

  virtual void printFunction(std::ostream &os, F f) const = 0;

  virtual std::string FtoString(F f) const {
    std::stringstream ss;
    printFunction(ss, f);
    return ss.str();
  }
};

} // namespace psr

#endif
