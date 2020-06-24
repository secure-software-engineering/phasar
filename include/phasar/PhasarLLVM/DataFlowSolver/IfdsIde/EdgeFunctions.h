/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractEdgeFunctions.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_

#include <iosfwd>
#include <iostream>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>

namespace psr {

template <typename AnalysisDomainTy,
          typename Container>
class MemoryManager;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>> class EdgeFunction {
public:
  using EdgeFunctionPtrType = EdgeFunction<AnalysisDomainTy, Container>*;
  using L = typename AnalysisDomainTy::l_t;

  virtual ~EdgeFunction() = default;

  virtual L computeTarget(L source) = 0;

  virtual EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType secondFunction, MemoryManager<AnalysisDomainTy, Container> memManager) = 0;

  virtual EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) = 0;

  virtual bool equal_to(EdgeFunctionPtrType other) const = 0;

  virtual void print(std::ostream &OS, bool isForDebug = false) const {
    OS << "EdgeFunction";
  }

  std::string str() {
    std::ostringstream oss;
    print(oss);
    return oss.str();
  }
};

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
static inline bool operator==(const EdgeFunction<AnalysisDomainTy, Container> &F,
                              const EdgeFunction<AnalysisDomainTy, Container> &G) {
  return F.equal_to(G);
}

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
static inline std::ostream &operator<<(std::ostream &OS,
                                       const EdgeFunction<AnalysisDomainTy, Container> &F) {
  F.print(OS);
  return OS;
}

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class AllTop : public EdgeFunction<AnalysisDomainTy, Container> {
public:
  using typename EdgeFunction<AnalysisDomainTy, Container>::EdgeFunctionPtrType;
  using typename EdgeFunction<AnalysisDomainTy, Container>::L;
private:
  const L topElement;

public:
  AllTop(L topElement) : topElement(topElement) {}

  ~AllTop() override = default;

  L computeTarget(L source) override { return topElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction, MemoryManager<AnalysisDomainTy, Container> memManager) override {
    return this;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    return otherFunction;
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    if (auto *alltop = dynamic_cast<AllTop<AnalysisDomainTy, Container> *>(other)) {
      return (alltop->topElement == topElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllTop";
  }
};

template <typename AnalysisDomainTy,
          typename Container> class EdgeIdentity;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class AllBottom : public EdgeFunction<AnalysisDomainTy, Container> {
public:
  using typename EdgeFunction<AnalysisDomainTy, Container>::EdgeFunctionPtrType;
  using typename EdgeFunction<AnalysisDomainTy, Container>::L;
private:
  const L bottomElement;

public:
  AllBottom(L bottomElement) : bottomElement(bottomElement) {}

  ~AllBottom() override = default;

  L computeTarget(L source) override { return bottomElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction, MemoryManager<AnalysisDomainTy, Container> memManager) override {
    if (auto *ab = dynamic_cast<AllBottom<AnalysisDomainTy, Container> *>(secondFunction)) {
      return this;
    }
    if (auto *ei = dynamic_cast<EdgeIdentity<AnalysisDomainTy, Container> *>(secondFunction)) {
      return this;
    }
    return secondFunction->composeWith(this);
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    if (otherFunction == this ||
        otherFunction->equal_to(this)) {
      return this;
    }
    if (auto *alltop = dynamic_cast<AllTop<AnalysisDomainTy, Container> *>(otherFunction)) {
      return this;
    }
    if (auto *ei = dynamic_cast<EdgeIdentity<AnalysisDomainTy, Container> *>(otherFunction)) {
      return this;
    }
    return this;
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    if (auto *allbottom = dynamic_cast<AllBottom<AnalysisDomainTy, Container> *>(other)) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllBottom";
  }
};

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class EdgeIdentity : public EdgeFunction<AnalysisDomainTy, Container> {
public:
  using typename EdgeFunction<AnalysisDomainTy, Container>::EdgeFunctionPtrType;
  using typename EdgeFunction<AnalysisDomainTy, Container>::L;

private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  ~EdgeIdentity() override = default;

  L computeTarget(L source) override { return source; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction, MemoryManager<AnalysisDomainTy, Container> memManager) override {
    return secondFunction;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    if ((otherFunction == this) ||
        otherFunction->equal_to(this)) {
      return this;
    }
    if (auto *ab = dynamic_cast<AllBottom<AnalysisDomainTy, Container> *>(otherFunction)) {
      return otherFunction;
    }
    if (auto *at = dynamic_cast<AllTop<AnalysisDomainTy, Container> *>(otherFunction)) {
      return this;
    }
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this);
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    return this == other;
  }

  static EdgeFunctionPtrType getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeFunctionPtrType instance(new EdgeIdentity<AnalysisDomainTy, Container>());
    return instance;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "EdgeIdentity";
  }
};

//===----------------------------------------------------------------------===//
// EdgeFunctions interface

template <typename AnalysisDomainTy> class EdgeFunctions {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using l_t = typename AnalysisDomainTy::l_t;

  using EdgeFunctionType = EdgeFunction<AnalysisDomainTy>;
  using EdgeFunctionPtrType = typename EdgeFunctionType::EdgeFunctionPtrType;

  virtual ~EdgeFunctions() = default;
  virtual EdgeFunctionPtrType getNormalEdgeFunction(n_t curr, d_t currNode,
                                                    n_t succ, d_t succNode) = 0;
  virtual EdgeFunctionPtrType getCallEdgeFunction(n_t callStmt, d_t srcNode,
                                                  f_t destinationFunction,
                                                  d_t destNode) = 0;
  virtual EdgeFunctionPtrType
  getReturnEdgeFunction(n_t callSite, f_t calleeFunction, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) = 0;
  virtual EdgeFunctionPtrType
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) = 0;
  virtual EdgeFunctionPtrType
  getSummaryEdgeFunction(n_t curr, d_t currNode, n_t succ, d_t succNode) = 0;
};

} // namespace psr

#endif
