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

  virtual void printNode(std::ostream &Os, N N) const = 0;

  virtual std::string ntoString(N N) const {
    std::stringstream Ss;
    printNode(Ss, N);
    return Ss.str();
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

  virtual void printDataFlowFact(std::ostream &Os, D D) const = 0;

  virtual std::string dtoString(D D) const {
    std::stringstream Ss;
    printDataFlowFact(Ss, D);
    return Ss.str();
  }
};

template <typename V> struct ValuePrinter {
  ValuePrinter() = default;
  ValuePrinter(const ValuePrinter &) = delete;
  ValuePrinter &operator=(const ValuePrinter &) = delete;
  ValuePrinter(ValuePrinter &&) = delete;
  ValuePrinter &operator=(ValuePrinter &&) = delete;
  virtual ~ValuePrinter() = default;

  virtual void printValue(std::ostream &Os, V V) const = 0;

  virtual std::string vtoString(V V) const {
    std::stringstream Ss;
    printValue(Ss, V);
    return Ss.str();
  }
};

template <typename T> struct TypePrinter {
  TypePrinter() = default;
  TypePrinter(const TypePrinter &) = delete;
  TypePrinter &operator=(const TypePrinter &) = delete;
  TypePrinter(TypePrinter &&) = delete;
  TypePrinter &operator=(TypePrinter &&) = delete;
  virtual ~TypePrinter() = default;

  virtual void printType(std::ostream &Os, T T) const = 0;

  virtual std::string ttoString(T T) const {
    std::stringstream Ss;
    printType(Ss, T);
    return Ss.str();
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

  virtual void printEdgeFact(std::ostream &Os, l_t L) const = 0;

  [[nodiscard]] virtual std::string ltoString(l_t L) const {
    std::stringstream Ss;
    printEdgeFact(Ss, L);
    return Ss.str();
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

  virtual void printFunction(std::ostream &Os, F F) const = 0;

  virtual std::string ftoString(F F) const {
    std::stringstream Ss;
    printFunction(Ss, F);
    return Ss.str();
  }
};

template <typename ContainerTy> struct ContainerPrinter {
  ContainerPrinter() = default;
  ContainerPrinter(const ContainerPrinter &) = delete;
  ContainerPrinter &operator=(const ContainerPrinter &) = delete;
  ContainerPrinter(ContainerPrinter &&) = delete;
  ContainerPrinter &operator=(ContainerPrinter &&) = delete;
  virtual ~ContainerPrinter() = default;

  virtual void printContainer(std::ostream &Os, ContainerTy C) const = 0;

  virtual std::string containertoString(ContainerTy C) const {
    std::stringstream Ss;
    printContainer(Ss, C);
    return Ss.str();
  }
};

} // namespace psr

#endif
