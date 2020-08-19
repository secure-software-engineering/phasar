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

//
// This class models a flow function for distributive data-flow problems.
//
template <typename D, typename Container = std::set<D>> class FlowFunction {
  static_assert(std::is_same<typename Container::value_type, D>::value,
                "Container values needs to be the same as D");

public:
  using FlowFunctionType = FlowFunction<D, Container>;
  using FlowFunctionPtrType = std::shared_ptr<FlowFunctionType>;

  using container_type = Container;
  using value_type = typename container_type::value_type;

  virtual ~FlowFunction() = default;

  //
  // This function is called for each data-flow fact Source that holds before
  // the instruction under analysis. The return value is a (potentially empty)
  // set of data-flow facts that are generated from Source and hold after the
  // instruction under analysis. In other words: the function describes what
  // exploded supergraph edges have to be "drawn".
  //
  // Please also refer to the various flow function factories of the
  // FlowFunctions interface: FlowFunctions::get*FlowFunction() for more
  // details.
  //
  virtual container_type computeTargets(D Source) = 0;
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

  //
  // Describes the effects of the current instruction, i.e. data-flows, along
  // normal (non-call, non-return) instructions. Analysis writers are free to
  // inspect the successor instructions, too, as a lookahead.
  //
  // Let instruction_1 := Curr, instruction_2 := Succ, and 0 the tautological
  // lambda fact.
  //
  // The returned flow function implementation f
  // (FlowFunction::computeTargets()) is applied to each data-flow fact d_i that
  // holds before the current statement under analysis. f's return type is a set
  // of (target) facts that have to be generated from the source fact d_i by the
  // data-flow solver. Each combination of input fact d_i (given as an input to
  // f) and respective output facts (f(d_i)) represents an edge that must be
  // "drawn" to construct the exploded supergraph for the analysis problem to be
  // solved.
  //
  // The concrete implementation of f is depending on the analysis problem. In
  // the following, we present a brief, contrived example:
  //
  // f is applied to each data-flow fact d_i that holds before instruction_1. We
  // assume that f is implemented to produce the following outputs.
  //
  //    f(0) -> {0}       // pass the lambda (or zero fact) as identity
  //    f(o) -> {o, x}    // generate a new fact x from o
  //    f(.) -> {.}       // pass all other facts that hold before instruction_1
  //                      // as identity
  //
  // The above implementation corresponds to the following edges in the exploded
  // supergraph.
  //
  //                         0  o      ...
  //                         |  |\     ...
  // x = instruction_1 o p   |  | \    ...
  //                         |  |  |   ...
  //                         v  v  v   ...
  //                         0  o  x   ...
  //
  // y = instruction_2 q r
  //
  virtual FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) = 0;

  //
  // Handles call flows: describes the effects of a function call at callInst
  // to the callee target destFun. If a call instruction has multiple callee
  // targets, for instance, because it is an indirect function call that cannot
  // be analyzed precisely in a static manner, the call flow function will be
  // queried for each callee target.
  //
  // This flow function usually handles parameter passing and maps actual to
  // formal parameters. If an analysis writer does not wish to analyze a given
  // callee target they can return a flow function implementation that kills all
  // data-flow facts (e.g. KillAll) such that call is not followed. A commonly
  // used trick to model the effects of functions that are not present (e.g.
  // library functions such as malloc(), free(), etc.) is to kill all facts at
  // the call to the respective target and plugin the semantics in the
  // call-to-return flow function. In the call-to-return flow function, an
  // analysis writer can check if the function of interest is one of the
  // possible targets and then, return a flow function implementation that
  // describes the special semantics of that function call.
  //
  // Let start_point be the starting point of the callee target CalleeFun.
  //
  // The returned flow function implementation f
  // (FlowFunction::computeTargets()) is applied to each data-flow fact d_i that
  // holds right before the CallInst. f's return type is a set
  // of (target) facts that have to be generated from the source fact d_i by the
  // data-flow solver. Each target fact that is generated will hold before
  // start_point.
  //
  // The concrete implementation of f is depending on the analysis problem. In
  // the following, we present a brief, contrived example:
  //
  // f is applied to each data-flow fact d_i that holds before CallInst. We
  // assume that f is implemented to produce the following outputs.
  //
  //    f(0) -> {0}       // pass as identity into the callee target
  //    f(o) -> {q}       // map actual o into formal q
  //    f(p) -> {r}       // map actual p into formal r
  //    f(.) -> {}        // kill all other facts that are not visible to the
  //                      // callee target
  //
  // The above implementation corresponds to the following edges in the exploded
  // supergraph.
  //
  //                            0  o  p   ...
  //                             \  \  \  ...
  // x = CalleeFun(o, p, ...)     \  \  +----------------+
  //                               \  +----------------  |
  //                                 +-------------+  +  |
  //                                      ...      |  |  |
  //                                      ...      |  |  |
  //                            0  o  p   ...      |  |  |
  //                                               |  |  |
  //                                               |  |  |
  //                                               |  |  |
  //                     Ty CalleeFun(q, r, ...)   |  |  |
  //                                               v  v  v
  //                                               0  q  r   ...
  //
  //                                 start point
  //
  virtual FlowFunctionPtrType getCallFlowFunction(n_t CallInst,
                                                  f_t CalleeFun) = 0;

  //
  // Handles return flows: describes the data-flows from an ExitInst to the
  // corresponding RetSite.
  //
  // This flow function usually handles the returned value of the callee target
  // as well as the parameter mapping back to the caller of CalleeFun for
  // pointer parameters as modifications made by CalleeFun are visible to the
  // caller. Data-flow facts that are not returned or escape via function
  // pointer parameters (or global variables) are usually killed.
  //
  // The returned flow function implementation f
  // (FlowFunction::computeTargets()) is applied to each data-flow fact d_i that
  // holds right before the ExitInst. f's return type is a set
  // of (target) facts that have to be generated from the source fact d_i by the
  // data-flow solver. Each target fact that is generated will hold after
  // CallSite.
  //
  // The concrete implementation of f is depending on the analysis problem. In
  // the following, we present a brief, contrived example:
  //
  // f is applied to each data-flow fact d_i that holds before ExitInst. We
  // assume that f is implemented to produce the following outputs.
  //
  //    f(0) -> {0}       // pass as identity into the callee target
  //    f(r) -> {x}       // map return value to lhs variable at CallSite
  //    f(q) -> {o}       // map pointer-typed formal q to actual o
  //    f(.) -> {}        // kill all other facts that are not visible to the
  //                      // caller
  //
  // The above implementation corresponds to the following edges in the exploded
  // supergraph.
  //
  //                         0  o   ...
  //
  // x = CalleeFun(o, ...)
  //                               +------------------+
  //                            +--|---------------+  |
  //                         +--|--|------------+  |  |
  //                         v  v  v   ...      |  |  |
  //                         0  o  x   ...      |  |  |
  //                                            |  |  |
  //                                            |  |  |
  //                                            |  |  |
  //                     Ty CalleeFun(q, ...)   |  |  |
  //                                            |  |  |
  //                                            0  q  r   ...
  //
  //                                 return r
  //
  virtual FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                                 n_t ExitInst, n_t RetSite) = 0;

  //
  // Describes the data-flows alongsite a CallSite.
  //
  // This flow function usually passes all data-flow facts that are not involved
  // in the function call alongsite the CallSite. Data-flow facts that are not
  // actual parameters or passed by value, modifications to those within a
  // callee are not visible in the caller context, are mostly passed as
  // identity. The call-to-return flow function may also be used to describe
  // special semantics (cf. getCallFlowFunction()).
  //
  // The returned flow function implementation f
  // (FlowFunction::computeTargets()) is applied to each data-flow fact d_i that
  // holds right before the CallSite. f's return type is a set
  // of (target) facts that have to be generated from the source fact d_i by the
  // data-flow solver. Each target fact that is generated will hold after
  // CallSite.
  //
  // The concrete implementation of f is depending on the analysis problem. In
  // the following, we present a brief, contrived example:
  //
  // f is applied to each data-flow fact d_i that holds before CallSite. We
  // assume that f is implemented to produce the following outputs.
  //
  //    f(0) -> {0}       // pass lambda as identity alongsite the CallSite
  //    f(o) -> {o}       // assuming that o is passed by value, it is passed
  //                      // alongsite the CallSite
  //    f(p) -> {}        // assuming that p is a pointer-typed value, we need
  //                      // to kill p, as it will be handled by the call- and
  //                      // return-flow functions
  //    f(.) -> {.}       // pass everything that is not involved in the call as
  //                      // identity
  //
  // The above implementation corresponds to the following edges in the exploded
  // supergraph.
  //
  //                            0  o   ...
  //                            |  |
  //                            |  +-------+
  //                            +--------+ |
  //                                     | |
  // x = CalleeFun(o, p, ...)            | |
  //                                     | |
  //                            +--------+ |
  //                            |  +-------+
  //                            v  v
  //                            0  o  x   ...
  //
  virtual FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           std::set<f_t> Callees) = 0;

  //
  // May be used to encode special sementics of a given callee target (whose
  // call should not be directly followed by the data-flow solver) similar to
  // the getCallFlowFunction() --> getCallToRetFlowFunction() trick (cf.
  // getCallFlowFunction()).
  //
  // The default implementation returns a nullptr to indicate that the mechanism
  // should not be used.
  //
  virtual FlowFunctionPtrType getSummaryFlowFunction(n_t Curr,
                                                     f_t CalleeFun) = 0;
};
} // namespace  psr

#endif
