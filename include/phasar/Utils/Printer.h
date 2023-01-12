/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_PRINTER_H
#define PHASAR_PHASARLLVM_UTILS_PRINTER_H

#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <string>

namespace psr {

template <typename P, typename T>
std::string toStringBuilder(void (P::*Printer)(llvm::raw_ostream &, T) const,
                            const P *This, const T &Arg) {
  std::string Buffer;
  llvm::raw_string_ostream StrS(Buffer);
  std::invoke(Printer, This, std::ref(StrS), Arg);
  return Buffer;
}

template <typename N> struct NodePrinterBase {

  virtual void printNode(llvm::raw_ostream &OS, N Stmt) const = 0;

  [[nodiscard]] std::string NtoString(N Stmt) const { // NOLINT
    return toStringBuilder(&NodePrinterBase::printNode, this, Stmt);
  }
};
template <typename AnalysisDomainTy>
using NodePrinter = NodePrinterBase<typename AnalysisDomainTy::n_t>;

template <typename D> struct DataFlowFactPrinterBase {
  virtual void printDataFlowFact(llvm::raw_ostream &OS, D Fact) const = 0;

  [[nodiscard]] std::string DtoString(D Fact) const { // NOLINT
    return toStringBuilder(&DataFlowFactPrinterBase::printDataFlowFact, this,
                           Fact);
  }
};
template <typename AnalysisDomainTy>
using DataFlowFactPrinter =
    DataFlowFactPrinterBase<typename AnalysisDomainTy::d_t>;

template <typename V> struct ValuePrinter {
  virtual void printValue(llvm::raw_ostream &OS, V Val) const = 0;

  [[nodiscard]] std::string VtoString(V Val) const { // NOLINT
    return toStringBuilder(&ValuePrinter::printValue, this, Val);
  }
};

template <typename T> struct TypePrinter {
  virtual void printType(llvm::raw_ostream &OS, T Ty) const = 0;

  [[nodiscard]] std::string TtoString(T Ty) const { // NOLINT
    return toStringBuilder(&TypePrinter::printType, this, Ty);
  }
};

template <typename AnalysisDomainTy> struct EdgeFactPrinter {
  using l_t = typename AnalysisDomainTy::l_t;

  virtual void printEdgeFact(llvm::raw_ostream &OS, l_t L) const = 0;

  [[nodiscard]] std::string LtoString(l_t L) const { // NOLINT
    return toStringBuilder(&EdgeFactPrinter::printEdgeFact, this, L);
  }
};

template <typename AnalysisDomainTy> struct FunctionPrinter {
  using F = typename AnalysisDomainTy::f_t;

  virtual void printFunction(llvm::raw_ostream &OS, F Func) const = 0;

  [[nodiscard]] std::string FtoString(F Func) const { // NOLINT
    return toStringBuilder(&FunctionPrinter::printFunction, this, Func);
  }
};

template <typename ContainerTy> struct ContainerPrinter {
  virtual void printContainer(llvm::raw_ostream &OS,
                              ContainerTy Container) const = 0;

  [[nodiscard]] std::string
  ContainertoString(ContainerTy Container) const { // NOLINT
    return toStringBuilder(&ContainerPrinter::printContainer, this, Container);
  }
};

} // namespace psr

#endif