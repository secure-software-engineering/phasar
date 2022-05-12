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

  virtual void printNode(llvm::raw_ostream &OS, N Stmt) const = 0;

  virtual std::string NtoString(N Stmt) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printNode(StrS, Stmt);
    return StrS.str();
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

  virtual void printDataFlowFact(llvm::raw_ostream &OS, D Fact) const = 0;

  [[nodiscard]] virtual std::string DtoString(D Fact) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printDataFlowFact(StrS, Fact);
    return StrS.str();
  }
};

template <typename V> struct ValuePrinter {
  ValuePrinter() = default;
  ValuePrinter(const ValuePrinter &) = delete;
  ValuePrinter &operator=(const ValuePrinter &) = delete;
  ValuePrinter(ValuePrinter &&) = delete;
  ValuePrinter &operator=(ValuePrinter &&) = delete;
  virtual ~ValuePrinter() = default;

  virtual void printValue(llvm::raw_ostream &OS, V Val) const = 0;

  virtual std::string VtoString(V Val) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printValue(StrS, Val);
    return StrS.str();
  }
};

template <typename T> struct TypePrinter {
  TypePrinter() = default;
  TypePrinter(const TypePrinter &) = delete;
  TypePrinter &operator=(const TypePrinter &) = delete;
  TypePrinter(TypePrinter &&) = delete;
  TypePrinter &operator=(TypePrinter &&) = delete;
  virtual ~TypePrinter() = default;

  virtual void printType(llvm::raw_ostream &OS, T Ty) const = 0;

  virtual std::string TtoString(T Ty) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printType(StrS, Ty);
    return StrS.str();
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

  virtual void printEdgeFact(llvm::raw_ostream &OS, l_t L) const = 0;

  [[nodiscard]] virtual std::string LtoString(l_t L) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printEdgeFact(StrS, L);
    return StrS.str();
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

  virtual void printFunction(llvm::raw_ostream &OS, F Func) const = 0;

  virtual std::string FtoString(F Func) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printFunction(StrS, Func);
    return StrS.str();
  }
};

template <typename ContainerTy> struct ContainerPrinter {
  ContainerPrinter() = default;
  ContainerPrinter(const ContainerPrinter &) = delete;
  ContainerPrinter &operator=(const ContainerPrinter &) = delete;
  ContainerPrinter(ContainerPrinter &&) = delete;
  ContainerPrinter &operator=(ContainerPrinter &&) = delete;
  virtual ~ContainerPrinter() = default;

  virtual void printContainer(llvm::raw_ostream &OS,
                              ContainerTy Container) const = 0;

  virtual std::string ContainertoString(ContainerTy Container) const { // NOLINT
    std::string Buffer;
    llvm::raw_string_ostream StrS(Buffer);
    printContainer(StrS, Container);
    return StrS.str();
  }
};

} // namespace psr

#endif
