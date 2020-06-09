/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * FlowFunctions.h
 *
 *  Created on: 03.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_H_

#include <functional>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace psr {

//===----------------------------------------------------------------------===//
//                              FlowFunction Class
//===----------------------------------------------------------------------===//

template <typename D, typename Container = std::set<D>> class FlowFunction {
  static_assert(std::is_same<typename Container::value_type, D>::value,
                "Container values needs to be the same as D");

public:
  using FlowFunctionType = FlowFunction<D, Container>;
  using FlowFunctionPtrType = std::shared_ptr<FlowFunctionType>;

  using container_type = Container;
  using value_type = typename container_type::value_type;

  virtual ~FlowFunction() = default;
  virtual container_type computeTargets(D source) = 0;
};

template <typename D, typename Container = std::set<D>>
class Identity : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  using typename FlowFunction<D, Container>::container_type;

  virtual ~Identity() = default;
  Identity(const Identity &i) = delete;
  Identity &operator=(const Identity &i) = delete;
  // simply return what the user provides
  container_type computeTargets(D source) override { return {source}; }
  static std::shared_ptr<Identity> getInstance() {
    static std::shared_ptr<Identity> instance =
        std::shared_ptr<Identity>(new Identity);
    return instance;
  }

private:
  Identity() = default;
};

template <typename D, typename Container = std::set<D>>
class LambdaFlow : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  LambdaFlow(std::function<container_type(D)> f) : flow(std::move(f)) {}
  virtual ~LambdaFlow() = default;
  container_type computeTargets(D source) override { return flow(source); }

private:
  std::function<container_type(D)> flow;
};

template <typename D, typename Container = std::set<D>>
class Compose : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  using typename FlowFunction<D, Container>::container_type;

  Compose(const std::vector<FlowFunction<D>> &funcs) : funcs(funcs) {}

  virtual ~Compose() = default;

  container_type computeTargets(const D &source) override {
    container_type current(source);
    for (const FlowFunctionType &func : funcs) {
      container_type next;
      for (const D &d : current) {
        container_type target = func.computeTargets(d);
        next.insert(target.begin(), target.end());
      }
      current = next;
    }
    return current;
  }

  static FlowFunctionPtrType
  compose(const std::vector<FlowFunctionType> &funcs) {
    std::vector<FlowFunctionType> vec;
    for (const FlowFunctionType &func : funcs) {
      if (func != Identity<D, Container>::getInstance()) {
        vec.insert(func);
      }
    }
    if (vec.size == 1) {
      return vec[0];
    } else if (vec.empty()) {
      return Identity<D, Container>::getInstance();
    }
    return std::make_shared<Compose>(vec);
  }

protected:
  const std::vector<FlowFunctionType> funcs;
};

//===----------------------------------------------------------------------===//
// Gen flow functions

template <typename D, typename Container = std::set<D>>
class Gen : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;

protected:
  D genValue;
  D zeroValue;

public:
  Gen(D genValue, D zeroValue) : genValue(genValue), zeroValue(zeroValue) {}
  virtual ~Gen() = default;

  container_type computeTargets(D source) override {
    if (source == zeroValue) {
      return {source, genValue};
    } else {
      return {source};
    }
  }
};

/**
 * @brief Generates the given value if the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be generated.
 */
template <typename D, typename Container = std::set<D>>
class GenIf : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenIf(D GenValue, std::function<bool(D)> Predicate)
      : GenValues({GenValue}), Predicate(Predicate) {}

  GenIf(container_type GenValues, std::function<bool(D)> Predicate)
      : GenValues(std::move(GenValues)), Predicate(Predicate) {}

  virtual ~GenIf() = default;

  container_type computeTargets(D Source) override {
    if (Predicate(Source)) {
      container_type ToGenerate;
      ToGenerate.insert(Source);
      ToGenerate.insert(GenValues.begin(), GenValues.end());
      return ToGenerate;
    } else {
      return {Source};
    }
  }

protected:
  container_type GenValues;
  std::function<bool(D)> Predicate;
};

template <typename D, typename Container = std::set<D>>
class GenAll : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenAll(container_type genValues, D zeroValue)
      : genValues(genValues), zeroValue(zeroValue) {}
  virtual ~GenAll() = default;
  container_type computeTargets(D source) override {
    if (source == zeroValue) {
      genValues.insert(source);
      return genValues;
    } else {
      return {source};
    }
  }

protected:
  container_type genValues;
  D zeroValue;
};

//===----------------------------------------------------------------------===//
// Kill flow functions

template <typename D, typename Container = std::set<D>>
class Kill : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  Kill(D killValue) : killValue(killValue) {}
  virtual ~Kill() = default;
  container_type computeTargets(D source) override {
    if (source == killValue) {
      return {};
    } else {
      return {source};
    }
  }

protected:
  D killValue;
};

/**
 * @brief Kills all facts for which the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be killed.
 */
template <typename D, typename Container = std::set<D>>
class KillIf : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  KillIf(std::function<bool(D)> Predicate) : Predicate(Predicate) {}
  virtual ~KillIf() = default;
  container_type computeTargets(D source) override {
    if (Predicate(source)) {
      return {};
    } else {
      return {source};
    }
  }

protected:
  std::function<bool(D)> Predicate;
};

template <typename D, typename Container = std::set<D>>
class KillMultiple : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  KillMultiple(std::set<D> killValues) : killValues(killValues) {}
  virtual ~KillMultiple() = default;
  container_type computeTargets(D source) override {
    if (killValues.find(source) != killValues.end()) {
      return {};
    } else {
      return {source};
    }
  }

protected:
  container_type killValues;
};

template <typename D, typename Container = std::set<D>>
class KillAll : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  virtual ~KillAll() = default;
  KillAll(const KillAll &k) = delete;
  KillAll &operator=(const KillAll &k) = delete;
  container_type computeTargets(D source) override { return container_type(); }
  static std::shared_ptr<KillAll<D>> getInstance() {
    static std::shared_ptr<KillAll> instance =
        std::shared_ptr<KillAll>(new KillAll);
    return instance;
  }

private:
  KillAll() = default;
};

template <typename D, typename Container = std::set<D>>
class Transfer : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  Transfer(D toValue, D fromValue) : toValue(toValue), fromValue(fromValue) {}
  virtual ~Transfer() = default;
  container_type computeTargets(D source) override {
    if (source == fromValue) {
      return {source, toValue};
    } else if (source == toValue) {
      return {};
    } else {
      return {source};
    }
  }

protected:
  D toValue;
  D fromValue;
};

template <typename D, typename Container = std::set<D>>
class Union : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  using typename FlowFunction<D, Container>::container_type;

  Union(const std::vector<FlowFunctionType> &funcs) : funcs(funcs) {}
  virtual ~Union() = default;
  container_type computeTargets(const D &source) override {
    container_type result;
    for (const FlowFunctionType &func : funcs) {
      container_type target = func.computeTarget(source);
      result.insert(target.begin(), target.end());
    }
    return result;
  }
  static FlowFunctionType setunion(const std::vector<FlowFunctionType> &funcs) {
    std::vector<FlowFunctionType> vec;
    for (const FlowFunctionType &func : funcs) {
      if (func != Identity<D, Container>::getInstance()) {
        vec.add(func);
      }
    }
    if (vec.size() == 1) {
      return vec[0];
    } else if (vec.empty()) {
      return Identity<D, Container>::getInstance();
    }
    return Union(vec);
  }

protected:
  const std::vector<FlowFunctionType> funcs;
};

template <typename D, typename Container = std::set<D>>
class ZeroedFlowFunction : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

public:
  ZeroedFlowFunction(FlowFunctionPtrType ff, D zv)
      : delegate(ff), zerovalue(zv) {}
  container_type computeTargets(D source) override {
    if (source == zerovalue) {
      container_type result = delegate->computeTargets(source);
      result.insert(zerovalue);
      return result;
    } else {
      return delegate->computeTargets(source);
    }
  }

private:
  FlowFunctionPtrType delegate;
  D zerovalue;
};

//===----------------------------------------------------------------------===//
//                             FlowFunctions Class
//===----------------------------------------------------------------------===//

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class FlowFunctions {
  static_assert(
      std::is_same<typename Container::value_type,
                   typename AnalysisDomainTy::d_t>::value,
      "Value type of the container needs to match the type parameter D");

public:
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using n_t = typename AnalysisDomainTy::n_t;

  using FlowFunctionType = FlowFunction<d_t, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using container_type = typename FlowFunctionType::container_type;

  virtual ~FlowFunctions() = default;
  virtual FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) = 0;
  virtual FlowFunctionPtrType getCallFlowFunction(n_t callStmt,
                                                  f_t destFun) = 0;
  virtual FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                                 n_t exitStmt, n_t retSite) = 0;
  virtual FlowFunctionPtrType
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) = 0;
  virtual FlowFunctionPtrType getSummaryFlowFunction(n_t curr, f_t destFun) = 0;
};
} // namespace  psr

#endif
