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
  using container_type = Container;
  using value_type = typename container_type::value_type;

  virtual ~FlowFunction() = default;
  virtual container_type computeTargets(D source) = 0;
};

template <typename D> class Identity : public FlowFunction<D> {
private:
  Identity() = default;

public:
  virtual ~Identity() = default;
  Identity(const Identity &i) = delete;
  Identity &operator=(const Identity &i) = delete;
  // simply return what the user provides
  std::set<D> computeTargets(D source) override { return {source}; }
  static std::shared_ptr<Identity> getInstance() {
    static std::shared_ptr<Identity> instance =
        std::shared_ptr<Identity>(new Identity);
    return instance;
  }
};

template <typename D, typename Container = std::set<D>>
class LambdaFlow : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;

  std::function<container_type(D)> flow;

public:
  LambdaFlow(std::function<Container(D)> f) : flow(std::move(f)) {}
  virtual ~LambdaFlow() = default;
  container_type computeTargets(D source) override { return flow(source); }
};

template <typename D> class Compose : public FlowFunction<D> {
protected:
  const std::vector<FlowFunction<D>> funcs;

public:
  Compose(const std::vector<FlowFunction<D>> &funcs) : funcs(funcs) {}

  virtual ~Compose() = default;

  std::set<D> computeTargets(const D &source) override {
    std::set<D> current(source);
    for (const FlowFunction<D> &func : funcs) {
      std::set<D> next;
      for (const D &d : current) {
        std::set<D> target = func.computeTargets(d);
        next.insert(target.begin(), target.end());
      }
      current = next;
    }
    return current;
  }

  static std::shared_ptr<FlowFunction<D>>
  compose(const std::vector<FlowFunction<D>> &funcs) {
    std::vector<FlowFunction<D>> vec;
    for (const FlowFunction<D> &func : funcs) {
      if (func != Identity<D>::getInstance()) {
        vec.insert(func);
      }
    }
    if (vec.size == 1) {
      return vec[0];
    } else if (vec.empty()) {
      return Identity<D>::getInstance();
    }
    return std::make_shared<Compose>(vec);
  }
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
template <typename D> class GenIf : public FlowFunction<D> {
protected:
  std::set<D> GenValues;
  std::function<bool(D)> Predicate;

public:
  GenIf(D GenValue, std::function<bool(D)> Predicate)
      : GenValues({GenValue}), Predicate(Predicate) {}

  GenIf(std::set<D> GenValues, std::function<bool(D)> Predicate)
      : GenValues(std::move(GenValues)), Predicate(Predicate) {}

  virtual ~GenIf() = default;

  std::set<D> computeTargets(D Source) override {
    if (Predicate(Source)) {
      std::set<D> ToGenerate;
      ToGenerate.insert(Source);
      ToGenerate.insert(GenValues.begin(), GenValues.end());
      return ToGenerate;
    } else {
      return {Source};
    }
  }
};

template <typename D> class GenAll : public FlowFunction<D> {
protected:
  std::set<D> genValues;
  D zeroValue;

public:
  GenAll(std::set<D> genValues, D zeroValue)
      : genValues(genValues), zeroValue(zeroValue) {}
  virtual ~GenAll() = default;
  std::set<D> computeTargets(D source) override {
    if (source == zeroValue) {
      genValues.insert(source);
      return genValues;
    } else {
      return {source};
    }
  }
};

//===----------------------------------------------------------------------===//
// Kill flow functions

template <typename D> class Kill : public FlowFunction<D> {
protected:
  D killValue;

public:
  Kill(D killValue) : killValue(killValue) {}
  virtual ~Kill() = default;
  std::set<D> computeTargets(D source) override {
    if (source == killValue) {
      return {};
    } else {
      return {source};
    }
  }
};

/**
 * @brief Kills all facts for which the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be killed.
 */
template <typename D> class KillIf : public FlowFunction<D> {
protected:
  std::function<bool(D)> Predicate;

public:
  KillIf(std::function<bool(D)> Predicate) : Predicate(Predicate) {}
  virtual ~KillIf() = default;
  std::set<D> computeTargets(D source) override {
    if (Predicate(source)) {
      return {};
    } else {
      return {source};
    }
  }
};

template <typename D> class KillMultiple : public FlowFunction<D> {
protected:
  std::set<D> killValues;

public:
  KillMultiple(std::set<D> killValues) : killValues(killValues) {}
  virtual ~KillMultiple() = default;
  std::set<D> computeTargets(D source) override {
    if (killValues.find(source) != killValues.end()) {
      return {};
    } else {
      return {source};
    }
  }
};

template <typename D> class KillAll : public FlowFunction<D> {
private:
  KillAll() = default;

public:
  virtual ~KillAll() = default;
  KillAll(const KillAll &k) = delete;
  KillAll &operator=(const KillAll &k) = delete;
  std::set<D> computeTargets(D source) override { return std::set<D>(); }
  static std::shared_ptr<KillAll<D>> getInstance() {
    static std::shared_ptr<KillAll> instance =
        std::shared_ptr<KillAll>(new KillAll);
    return instance;
  }
};

template <typename D> class Transfer : public FlowFunction<D> {
protected:
  D toValue;
  D fromValue;

public:
  Transfer(D toValue, D fromValue) : toValue(toValue), fromValue(fromValue) {}
  virtual ~Transfer() = default;
  std::set<D> computeTargets(D source) override {
    if (source == fromValue) {
      return {source, toValue};
    } else if (source == toValue) {
      return {};
    } else {
      return {source};
    }
  }
};

template <typename D> class Union : public FlowFunction<D> {
protected:
  const std::vector<FlowFunction<D>> funcs;

public:
  Union(const std::vector<FlowFunction<D>> &funcs) : funcs(funcs) {}
  virtual ~Union() = default;
  std::set<D> computeTargets(const D &source) override {
    std::set<D> result;
    for (const FlowFunction<D> &func : funcs) {
      std::set<D> target = func.computeTarget(source);
      result.insert(target.begin(), target.end());
    }
    return result;
  }
  static FlowFunction<D> setunion(const std::vector<FlowFunction<D>> &funcs) {
    std::vector<FlowFunction<D>> vec;
    for (const FlowFunction<D> &func : funcs) {
      if (func != Identity<D>::getInstance()) {
        vec.add(func);
      }
    }
    if (vec.size() == 1) {
      return vec[0];
    } else if (vec.empty()) {
      return Identity<D>::getInstance();
    }
    return Union(vec);
  }
};

template <typename D, typename Container = std::set<D>>
class ZeroedFlowFunction : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;

private:
  std::shared_ptr<FlowFunction<D, Container>> delegate;
  D zerovalue;

public:
  ZeroedFlowFunction(std::shared_ptr<FlowFunction<D, Container>> ff, D zv)
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
};

//===----------------------------------------------------------------------===//
//                             FlowFunctions Class
//===----------------------------------------------------------------------===//

template <typename N, typename D, typename F, typename Container = std::set<D>>
class FlowFunctions {
  static_assert(
      std::is_same<typename Container::value_type, D>::value,
      "Value type of the container needs to match the type parameter D");

public:
  using FlowFunctionType = FlowFunction<D, Container>;
  using FlowFunctionPtrType = std::shared_ptr<FlowFunction<D, Container>>;

  using container_type = typename FlowFunctionType::container_type;

  virtual ~FlowFunctions() = default;
  virtual FlowFunctionPtrType getNormalFlowFunction(N curr, N succ) = 0;
  virtual FlowFunctionPtrType getCallFlowFunction(N callStmt, F destFun) = 0;
  virtual FlowFunctionPtrType getRetFlowFunction(N callSite, F calleeFun,
                                                 N exitStmt, N retSite) = 0;
  virtual FlowFunctionPtrType getCallToRetFlowFunction(N callSite, N retSite,
                                                       std::set<F> callees) = 0;
  virtual FlowFunctionPtrType getSummaryFlowFunction(N curr, F destFun) = 0;
};
} // namespace  psr

#endif
