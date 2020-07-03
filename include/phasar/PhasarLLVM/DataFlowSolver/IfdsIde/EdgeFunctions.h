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

template <typename L> class EdgeFunction {
public:
  using EdgeFunctionPtrType = std::shared_ptr<EdgeFunction<L>>;

  virtual ~EdgeFunction() = default;

  virtual L computeTarget(L source) = 0;

  virtual EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType secondFunction) = 0;

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

template <typename L>
class AllTop : public EdgeFunction<L>,
               public std::enable_shared_from_this<AllTop<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  const L topElement;

public:
  AllTop(L topElement) : topElement(topElement) {}

  ~AllTop() override = default;

  L computeTarget(L source) override { return topElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction) override {
    return this->shared_from_this();
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    return otherFunction;
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    if (auto *alltop = dynamic_cast<AllTop<L> *>(other.get())) {
      return (alltop->topElement == topElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllTop";
  }
};

template <typename L> class EdgeIdentity;

template <typename L>
class AllBottom : public EdgeFunction<L>,
                  public std::enable_shared_from_this<AllBottom<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  const L bottomElement;

public:
  AllBottom(L bottomElement) : bottomElement(bottomElement) {}

  ~AllBottom() override = default;

  L computeTarget(L source) override { return bottomElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction) override {
    if (auto *ab = dynamic_cast<AllBottom<L> *>(secondFunction.get())) {
      return this->shared_from_this();
    }
    if (auto *ei = dynamic_cast<EdgeIdentity<L> *>(secondFunction.get())) {
      return this->shared_from_this();
    }
    return secondFunction->composeWith(this->shared_from_this());
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    if (otherFunction.get() == this ||
        otherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }
    if (auto *alltop = dynamic_cast<AllTop<L> *>(otherFunction.get())) {
      return this->shared_from_this();
    }
    if (auto *ei = dynamic_cast<EdgeIdentity<L> *>(otherFunction.get())) {
      return this->shared_from_this();
    }
    return this->shared_from_this();
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    if (auto *allbottom = dynamic_cast<AllBottom<L> *>(other.get())) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllBottom";
  }
};

template <typename L>
class EdgeIdentity : public EdgeFunction<L>,
                     public std::enable_shared_from_this<EdgeIdentity<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  ~EdgeIdentity() override = default;

  L computeTarget(L source) override { return source; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction) override {
    return secondFunction;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    if ((otherFunction.get() == this) ||
        otherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }
    if (auto *ab = dynamic_cast<AllBottom<L> *>(otherFunction.get())) {
      return otherFunction;
    }
    if (auto *at = dynamic_cast<AllTop<L> *>(otherFunction.get())) {
      return this->shared_from_this();
    }
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this->shared_from_this());
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    return this == other.get();
  }

  static EdgeFunctionPtrType getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeFunctionPtrType instance(new EdgeIdentity<L>());
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

  using EdgeFunctionType = EdgeFunction<l_t>;
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
