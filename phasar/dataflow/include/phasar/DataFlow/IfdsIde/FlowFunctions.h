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

#ifndef PHASAR_DATAFLOW_IFDSIDE_FLOWFUNCTIONS_H
#define PHASAR_DATAFLOW_IFDSIDE_FLOWFUNCTIONS_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/ArrayRef.h"

#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
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

/// Helper template to check at compile-time whether a type implements the
/// FlowFunction interface, no matter which data-flow fact type it uses.
///
/// Use is_flowfunction_v instead.
template <typename FF> struct IsFlowFunction {
  template <typename D, typename Container>
  static std::true_type test(const FlowFunction<D, Container> &);
  static std::false_type test(...) {}

  static constexpr bool value = // NOLINT
      std::is_same_v<std::true_type,
                     decltype(test(std::declval<const FF &>()))>;
};

/// Helper template to check at compile-time whether a type implements the
/// FlowFunction interface, no matter which data-flow fact type it uses.
template <typename FF>
static constexpr bool is_flowfunction_v = IsFlowFunction<FF>::value; // NOLINT

/// Given a flow-function type FF, returns a (smart) pointer type pointing to FF
template <typename FF, typename = std::enable_if_t<is_flowfunction_v<FF>>>
using FlowFunctionPtrTypeOf = std::shared_ptr<FF>;

/// Given a dataflow-fact type and optionally a container-type, returns a
/// (smart) pointer type pointing to a FlowFunction with the specified
/// flow-fact- and container type.
///
/// Equivalent to FlowFunctionPtrTypeOf<FlowFunction<D, Container>>
template <typename D, typename Container = std::set<D>>
using FlowFunctionPtrType = FlowFunctionPtrTypeOf<FlowFunction<D, Container>>;

/// A flow function that propagates all incoming facts unchanged.
///
/// Given a flow-function f = identityFlow(), then for all incoming
/// dataflow-facts x, f(x) = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///                   x1  x1  x3 ...
///                   |   |   |  ...
///  id-instruction   |   |   |  ...
///                   v   v   v  ...
///                   x1  x2  x3 ...
///
template <typename D, typename Container = std::set<D>> auto identityFlow() {
  struct IdFF final : public FlowFunction<D, Container> {
    Container computeTargets(D Source) override { return {std::move(Source)}; }
  };
  static auto TheIdentity = std::make_shared<IdFF>();

  return TheIdentity;
}

/// The most generic flow function. Invokes the passed function object F to
/// retrieve the desired data-flows.
///
/// So, given a flow function f = lambdaFlow(F), then for all incoming
/// dataflow-facts x, f(x) = F(x).
///
/// In the exploded supergraph it may look as follows:
///
///                 x
///                 |
///  inst           F
///           /  /  |  \  \  ...
///          v  v   v   v  v
///          x1 x2  x  x3 x4
///
template <typename D, typename Fn> auto lambdaFlow(Fn &&F) {
  using Container = std::invoke_result_t<Fn, D>;
  struct LambdaFlow final : public FlowFunction<D, Container> {
    LambdaFlow(Fn &&F) : Flow(std::forward<Fn>(F)) {}
    Container computeTargets(D Source) override {
      return std::invoke(Flow, std::move(Source));
    }

    [[no_unique_address]] std::decay_t<Fn> Flow;
  };

  return std::make_shared<LambdaFlow>(std::forward<Fn>(F));
}

//===----------------------------------------------------------------------===//
// Gen flow functions

/// A flow function that generates a new dataflow fact (FactToGenerate) if
/// called with an already known dataflow fact (From). All other facts are
/// propagated like with the identityFlow.
///
/// Given a flow function f = generateFlow(v, w), then for all incoming dataflow
/// facts x:
///   f(w) = {v, w},
///   f(x) = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///       x  w     u ...
///       |  |\    | ...
///  inst |  | \   | ...
///       v  v  v  v ...
///       x  w  v  u
///
/// \note If the FactToGenerate already holds at the beginning of the statement,
/// this flow function does not kill it. For IFDS analysis it makes no
/// difference, but in the case of IDE, the corresponding edge functions are
/// being joined together potentially lowing precition. If that is an issue, use
/// transferFlow instead.
template <typename D, typename Container = std::set<D>>
auto generateFlow(psr::type_identity_t<D> FactToGenerate, D From) {
  struct GenFrom final : public FlowFunction<D, Container> {
    GenFrom(D GenValue, D FromValue)
        : GenValue(std::move(GenValue)), FromValue(std::move(FromValue)) {}

    Container computeTargets(D Source) override {
      if (Source == FromValue) {
        return {std::move(Source), GenValue};
      }
      return {std::move(Source)};
    }

    D GenValue;
    D FromValue;
  };

  return std::make_shared<GenFrom>(std::move(FactToGenerate), std::move(From));
}

/// A flow function similar to generateFlow, that generates a new dataflow fact
/// (FactToGenerate), if the given Predicate evaluates to true on an incoming
/// dataflow fact
///
/// So, given a flow function f = generateFlowIf(v, p), for all incoming
/// dataflow facts x:
///   f(x) = {v, x}   if p(x) == true
///   f(x) = {x}      else.
///
template <typename D, typename Container = std::set<D>,
          typename Fn = psr::TrueFn,
          typename = std::enable_if_t<std::is_invocable_r_v<bool, Fn, D>>>
auto generateFlowIf(D FactToGenerate, Fn Predicate) {
  struct GenFlowIf final : public FlowFunction<D, Container> {
    GenFlowIf(D GenValue, Fn &&Predicate)
        : GenValue(std::move(GenValue)),
          Predicate(std::forward<Fn>(Predicate)) {}

    Container computeTargets(D Source) override {
      if (std::invoke(Predicate, Source)) {
        return {std::move(Source), GenValue};
      }
      return {std::move(Source)};
    }

    D GenValue;
    [[no_unique_address]] std::decay_t<Fn> Predicate;
  };

  return std::make_shared<GenFlowIf>(std::move(FactToGenerate),
                                     std::forward<Fn>(Predicate));
}

/// A flow function that generates multiple new dataflow facts (FactsToGenerate)
/// if called from an already known dataflow fact (From).
///
/// Given a flow function f = generateManyFlows({v1, v2, ..., vN}, w), for all
/// incoming dataflow facts x:
///   f(w) = {v1, v2, ..., vN, w}
///   f(x) = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///       x  w                u ...
///       |  |\  \ ... \      | ...
///  inst |  | \  \ ... \     | ...
///       v  v  v  v ... \    v ...
///       x  w  v1 v2 ... vN  u
///
template <typename D, typename Container = std::set<D>,
          typename Range = std::initializer_list<D>,
          typename = std::enable_if_t<is_iterable_over_v<Range, D>>>
auto generateManyFlows(Range &&FactsToGenerate, D From) {
  struct GenMany final : public FlowFunction<D, Container> {
    GenMany(Container &&GenValues, D FromValue)
        : GenValues(std::move(GenValues)), FromValue(std::move(FromValue)) {}

    Container computeTargets(D Source) override {
      if (Source == FromValue) {
        auto Ret = GenValues;
        Ret.insert(std::move(Source));
        return Ret;
      }
      return {std::move(Source)};
    }

    Container GenValues;
    D FromValue;
  };

  auto MakeContainer = [](Range &&Rng) -> Container {
    if constexpr (std::is_convertible_v<std::decay_t<Range>, Container>) {
      return std::forward<Range>(Rng);
    } else {
      Container C;
      for (auto &&Fact : Rng) {
        C.insert(std::forward<decltype(Fact)>(Fact));
      }
      return C;
    }
  };
  return std::make_shared<GenMany>(
      MakeContainer(std::forward<Range>(FactsToGenerate)), std::move(From));
}

//===----------------------------------------------------------------------===//
// Kill flow functions

/// A flow function that stops propagating a specific dataflow fact
/// (FactToKill).
///
/// Given a flow function f = killFlow(v), for all incoming dataflow facts x:
///   f(v) = {}
///   f(x) = {x}
///
/// In the exploded supergraph it may look as follows:
///
///           u  v  w ...
///           |  |  |
///  inst     |     |
///           v     v
///           u  v  w ...
///
template <typename D, typename Container = std::set<D>>
auto killFlow(D FactToKill) {
  struct KillFlow final : public FlowFunction<D, Container> {
    KillFlow(D KillValue) : KillValue(std::move(KillValue)) {}
    Container computeTargets(D Source) override {
      if (Source == KillValue) {
        return {};
      }
      return {std::move(Source)};
    }
    D KillValue;
  };

  return std::make_shared<KillFlow>(std::move(FactToKill));
}

/// A flow function similar to killFlow that stops propagating all dataflow
/// facts for that the given Predicate evaluates to true.
///
/// Given a flow function f = killFlowIf(p), for all incoming dataflow facts x:
///   f(x) = {}   if p(x) == true
///   f(x) = {x}  else.
///
template <typename D, typename Container = std::set<D>,
          typename Fn = psr::TrueFn,
          typename = std::enable_if_t<std::is_invocable_r_v<bool, Fn, D>>>
auto killFlowIf(Fn Predicate) {
  struct KillFlowIf final : public FlowFunction<D, Container> {
    KillFlowIf(Fn &&Predicate) : Predicate(std::forward<Fn>(Predicate)) {}

    Container computeTargets(D Source) override {
      if (std::invoke(Predicate, Source)) {
        return {};
      }
      return {std::move(Source)};
    }

    [[no_unique_address]] std::decay_t<Fn> Predicate;
  };

  return std::make_shared<KillFlowIf>(std::forward<Fn>(Predicate));
}

/// A flow function that stops propagating a specific set of dataflow facts
/// (FactsToKill).
///
/// Given a flow function f = killManyFlows({v1, v2, ..., vN}), for all incoming
/// dataflow facts x:
///   f(v1) = {}
///   f(v2) = {}
///   ...
///   f(vN) = {}
///   f(x)  = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///           u  v1  v2 ... vN  w ...
///           |  |   |       |  |
///  inst     |                 |
///           v                 v
///           u  v1  v2 ... vN  w ...
///
template <typename D, typename Container = std::set<D>,
          typename Range = std::initializer_list<D>,
          typename = std::enable_if_t<is_iterable_over_v<Range, D>>>
auto killManyFlows(Range &&FactsToKill) {
  struct KillMany final : public FlowFunction<D, Container> {
    KillMany(Container &&KillValues) : KillValues(std::move(KillValues)) {}

    Container computeTargets(D Source) override {
      if (KillValues.count(Source)) {
        return {};
      }
      return {std::move(Source)};
    }

    Container KillValues;
  };

  auto MakeContainer = [](Range &&Rng) -> Container {
    if constexpr (std::is_convertible_v<std::decay_t<Range>, Container>) {
      return std::forward<Range>(Rng);
    } else {
      Container C;
      for (auto &&Fact : Rng) {
        C.insert(std::forward<decltype(Fact)>(Fact));
      }
      return C;
    }
  };
  return std::make_shared<KillMany>(
      MakeContainer(std::forward<Range>(FactsToKill)));
}

/// A flow function that stops propagating *all* incoming dataflow facts.
///
/// Given a flow function f = killAllFlows(), for all incoming dataflow facts x,
/// f(x) = {}.
///
template <typename D, typename Container = std::set<D>> auto killAllFlows() {
  struct KillAllFF final : public FlowFunction<D, Container> {
    Container computeTargets(D Source) override { return {std::move(Source)}; }
  };
  static auto TheKillAllFlow = std::make_shared<KillAllFF>();

  return TheKillAllFlow;
}

//===----------------------------------------------------------------------===//
// Gen-and-kill flow functions

/// A flow function that composes kill and generate flow functions.
/// Like generateFlow it generates a new dataflow fact (FactToGenerate), if
/// called with a specific dataflow fact (From).
/// However, like killFlowIf it stops propagating all other dataflow facts.
///
/// Given a flow function f = generateFlowAndKillAllOthers(v, w), for all
/// incoming dataflow facts x:
///   f(w) = {v, w}
///   f(x) = {}.
///
/// Equivalent to: killFlowIf(λz.z!=w) o generateFlow(v, w) (where o denotes
/// function composition)
///
/// In the exploded supergraph it may look as follows:
///
///         x  w     u ...
///         |  |\    |
///  inst      | \     ...
///            v  v
///         x  w  v  u
///
template <typename D, typename Container = std::set<D>>
auto generateFlowAndKillAllOthers(psr::type_identity_t<D> FactToGenerate,
                                  D From) {
  struct GenFlowAndKillAllOthers final : public FlowFunction<D, Container> {
    GenFlowAndKillAllOthers(D GenValue, D FromValue)
        : GenValue(std::move(GenValue)), FromValue(std::move(FromValue)) {}

    Container computeTargets(D Source) override {
      if (Source == FromValue) {
        return {std::move(Source), GenValue};
      }
      return {};
    }

    D GenValue;
    D FromValue;
  };

  return std::make_shared<GenFlowAndKillAllOthers>(std::move(FactToGenerate),
                                                   std::move(From));
}

/// A flow function similar to generateFlowAndKillAllOthers that may generate
/// multiple dataflow facts (FactsToGenerate) is called with a specific fact
/// (From) and stops propagating all other dataflow facts.
///
/// Given a flow function f = generateManyFlowsAndKillAllOthers({v1, v2, ...,
/// vN}, w), for all incoming dataflow facts x:
///   f(w) = {v1, v2, ..., vN, w}
///   f(x) = {}.
///
/// In the exploded supergraph it may look as follows:
///
///       x  w                u ...
///       |  |\  \ ... \      | ...
///  inst    | \  \ ... \       ...
///          v  v  v ... \      ...
///       x  w  v1 v2 ... vN  u
///
template <typename D, typename Container = std::set<D>,
          typename Range = std::initializer_list<D>,
          typename = std::enable_if_t<is_iterable_over_v<Range, D>>>
auto generateManyFlowsAndKillAllOthers(Range &&FactsToGenerate, D From) {
  struct GenManyAndKillAllOthers final : public FlowFunction<D, Container> {
    GenManyAndKillAllOthers(Container &&GenValues, D FromValue)
        : GenValues(std::move(GenValues)), FromValue(std::move(FromValue)) {}

    Container computeTargets(D Source) override {
      if (Source == FromValue) {
        auto Ret = GenValues;
        Ret.insert(std::move(Source));
        return Ret;
      }
      return {};
    }

    Container GenValues;
    D FromValue;
  };

  auto MakeContainer = [](Range &&Rng) -> Container {
    if constexpr (std::is_convertible_v<std::decay_t<Range>, Container>) {
      return std::forward<Range>(Rng);
    } else {
      Container C;
      for (auto &&Fact : Rng) {
        C.insert(std::forward<decltype(Fact)>(Fact));
      }
      return C;
    }
  };
  return std::make_shared<GenManyAndKillAllOthers>(
      MakeContainer(std::forward<Range>(FactsToGenerate)), std::move(From));
}

//===----------------------------------------------------------------------===//
// Miscellaneous flow functions

/// A flow function that, similar to generateFlow, generates a new dataflow fact
/// (FactsToGenerate) when called with a specific dataflow fact (From).
/// Unlike generateFlow, it kills FactToGenerate if it is part of the incoming
/// facts. THis has no additional effect for IFDS analyses (which in fact should
/// use generateFlow instead), but for IDE analyses it may avoid joining the
/// edge functions reaching the FactToGenerate together which may improve the
/// analysis' precision.
///
/// Given a flow function f = transferFlow(v, w), for all incoming dataflow
/// facts x:
///   f(v) = {}
///   f(w) = {v, w}
///   f(x) = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///       x  w   v  u ...
///       |  |\  |  | ...
///       |  | \    | ...
///  inst |  |  \   | ...
///       v  v   v  v ...
///       x  w   v  u
///
template <typename D, typename Container = std::set<D>>
auto transferFlow(psr::type_identity_t<D> FactToGenerate, D From) {
  struct TransferFlow final : public FlowFunction<D, Container> {
    TransferFlow(D GenValue, D FromValue)
        : GenValue(std::move(GenValue)), FromValue(std::move(FromValue)) {}

    Container computeTargets(D Source) override {
      if (Source == FromValue) {
        return {std::move(Source), GenValue};
      }
      if (Source == GenValue) {
        return {};
      }
      return {std::move(Source)};
    }

    D GenValue;
    D FromValue;
  };

  return std::make_shared<TransferFlow>(std::move(FactToGenerate),
                                        std::move(From));
}

/// A flow function that takes two other flow functions OneFF and OtherFF and
/// applies both flow functions on each input merging the results together with
/// set-union.
///
/// Given a flow function f = unionFlows(g, h), for all incoming dataflow facts
/// x:
///   f(x) = g(x) u h(x).     (where u denotes set-union)
///
template <typename F1, typename F2, typename D = typename F1::value_type,
          typename Container = typename F1::container_type,
          typename = std::enable_if_t<
              std::is_same_v<D, typename F2::value_type> &&
              std::is_same_v<Container, typename F2::container_type>>>
auto unionFlows(FlowFunctionPtrTypeOf<F1> OneFF,
                FlowFunctionPtrTypeOf<F2> OtherFF) {
  struct UnionFlow final : public FlowFunction<D, Container> {
    UnionFlow(FlowFunctionPtrTypeOf<F1> OneFF,
              FlowFunctionPtrTypeOf<F2> OtherFF) noexcept
        : OneFF(std::move(OneFF)), OtherFF(std::move(OtherFF)) {}

    Container computeTargets(D Source) override {
      auto OneRet = OneFF->computeTargets(Source);
      auto OtherRet = OtherFF->computeTargets(std::move(Source));
      if (OneRet.size() < OtherRet.size()) {
        std::swap(OneRet, OtherRet);
      }

      OneRet.insert(std::make_move_iterator(OtherRet.begin()),
                    std::make_move_iterator(OtherRet.end()));
      return OneRet;
    }

    FlowFunctionPtrTypeOf<F1> OneFF;
    FlowFunctionPtrTypeOf<F2> OtherFF;
  };

  return std::make_shared<UnionFlow>(std::move(OneFF), std::move(OtherFF));
}

/// Wrapper flow function that is automatically used by the IDESolver if the
/// autoAddZero configuration option is set to true (default).
/// Ensures that the tautological zero-flow fact (Λ) does not get killed.
template <typename D, typename Container = std::set<D>>
class ZeroedFlowFunction : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

public:
  ZeroedFlowFunction(FlowFunctionPtrType FF, D ZV)
      : Delegate(std::move(FF)), ZeroValue(ZV) {}
  container_type computeTargets(D Source) override {
    if (Source == ZeroValue) {
      container_type Result = Delegate->computeTargets(Source);
      Result.insert(ZeroValue);
      return Result;
    }
    return Delegate->computeTargets(Source);
  }

private:
  FlowFunctionPtrType Delegate;
  D ZeroValue;
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
                           llvm::ArrayRef<f_t> Callees) = 0;

  //
  // May be used to encode special sementics of a given callee target (whose
  // call should not be directly followed by the data-flow solver) similar to
  // the getCallFlowFunction() --> getCallToRetFlowFunction() trick (cf.
  // getCallFlowFunction()).
  //
  // The default implementation returns a nullptr to indicate that the mechanism
  // should not be used.
  //
  virtual FlowFunctionPtrType getSummaryFlowFunction(n_t Curr, f_t CalleeFun) {
    return nullptr;
  }
};

////////////////////////////////////////////////////////////////////////////////
//                          Legacy Flow Functions
////////////////////////////////////////////////////////////////////////////////

template <typename D, typename Container = std::set<D>>
class Identity : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  using typename FlowFunction<D, Container>::container_type;

  ~Identity() override = default;
  Identity(const Identity &I) = delete;
  Identity &operator=(const Identity &I) = delete;
  // simply return what the user provides
  container_type computeTargets(D Source) override { return {Source}; }
  static std::shared_ptr<Identity> getInstance() {
    static std::shared_ptr<Identity> Instance =
        std::shared_ptr<Identity>(new Identity);
    return Instance;
  }

private:
  Identity() = default;
};

template <typename D, typename Fn, typename Container = std::set<D>>
class [[deprecated("Use lambdaFlow() instead")]] LambdaFlow
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  LambdaFlow(Fn && F) : Flow(std::move(F)) {}
  LambdaFlow(const Fn &F) : Flow(F) {}
  ~LambdaFlow() override = default;
  container_type computeTargets(D Source) override { return Flow(Source); }

private:
  // std::function<container_type(D)> flow;
  Fn Flow;
};

template <typename D, typename Fn, typename Container = std::set<D>>
typename FlowFunction<D>::FlowFunctionPtrType makeLambdaFlow(Fn &&F) {
  return std::make_shared<LambdaFlow<D, std::decay_t<Fn>, Container>>(
      std::forward<Fn>(F));
}

template <typename D, typename Container = std::set<D>>
class Compose : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  using typename FlowFunction<D, Container>::container_type;

  Compose(const std::vector<FlowFunction<D>> &Funcs) : Funcs(Funcs) {}

  ~Compose() override = default;

  container_type computeTargets(const D &Source) override {
    container_type Current(Source);
    for (const FlowFunctionType &Func : Funcs) {
      container_type Next;
      for (const D &Fact : Current) {
        container_type Target = Func.computeTargets(Fact);
        Next.insert(Target.begin(), Target.end());
      }
      Current = Next;
    }
    return Current;
  }

  static FlowFunctionPtrType
  compose(const std::vector<FlowFunctionType> &Funcs) {
    std::vector<FlowFunctionType> Vec;
    for (const FlowFunctionType &Func : Funcs) {
      if (Func != Identity<D, Container>::getInstance()) {
        Vec.insert(Func);
      }
    }
    if (Vec.size() == 1) { // NOLINT(readability-container-size-empty)
      return Vec[0];
    }
    if (Vec.empty()) {
      return Identity<D, Container>::getInstance();
    }
    return std::make_shared<Compose>(Vec);
  }

protected:
  const std::vector<FlowFunctionType> Funcs;
};

//===----------------------------------------------------------------------===//
// Gen flow functions

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use generateFlow() instead")]] Gen
    : public FlowFunction<D, Container> {
  using typename FlowFunction<D, Container>::container_type;

protected:
  D GenValue;
  D ZeroValue;

public:
  Gen(D GenValue, D ZeroValue) : GenValue(GenValue), ZeroValue(ZeroValue) {}
  ~Gen() override = default;

  container_type computeTargets(D Source) override {
    if (Source == ZeroValue) {
      return {Source, GenValue};
    }
    return {Source};
  }
};

/**
 * @brief Generates the given value if the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be generated.
 */
template <typename D, typename Container = std::set<D>>
class [[deprecated("Use generateFlowIf() instead")]] GenIf
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenIf(D GenValue, std::function<bool(D)> Predicate)
      : GenValues({GenValue}), Predicate(std::move(Predicate)) {}

  GenIf(container_type GenValues, std::function<bool(D)> Predicate)
      : GenValues(std::move(GenValues)), Predicate(std::move(Predicate)) {}

  ~GenIf() override = default;

  container_type computeTargets(D Source) override {
    if (Predicate(Source)) {
      container_type ToGenerate;
      ToGenerate.insert(Source);
      ToGenerate.insert(GenValues.begin(), GenValues.end());
      return ToGenerate;
    }
    return {Source};
  }

protected:
  container_type GenValues;
  std::function<bool(D)> Predicate;
};

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use generateManyFlows() instead")]] GenAll
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenAll(container_type GenValues, D ZeroValue)
      : GenValues(std::move(GenValues)), ZeroValue(ZeroValue) {}
  ~GenAll() override = default;
  container_type computeTargets(D Source) override {
    if (Source == ZeroValue) {
      GenValues.insert(Source);
      return GenValues;
    }
    return {Source};
  }

protected:
  container_type GenValues;
  D ZeroValue;
};

//===----------------------------------------------------------------------===//
// Kill flow functions

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use killFlow() instead")]] Kill
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  Kill(D KillValue) : KillValue(KillValue) {}
  ~Kill() override = default;
  container_type computeTargets(D Source) override {
    if (Source == KillValue) {
      return {};
    }
    return {Source};
  }

protected:
  D KillValue;
};

/// \brief Kills all facts for which the given predicate evaluates to true.
/// \tparam D The type of data-flow facts to be killed.
template <typename D, typename Container = std::set<D>>
class [[deprecated("Use killFlowIf instead")]] KillIf
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  KillIf(std::function<bool(D)> Predicate) : Predicate(std::move(Predicate)) {}
  ~KillIf() override = default;
  container_type computeTargets(D Source) override {
    if (Predicate(Source)) {
      return {};
    }
    return {Source};
  }

protected:
  std::function<bool(D)> Predicate;
};

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use killManyFlows() instead")]] KillMultiple
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  KillMultiple(std::set<D> KillValues) : KillValues(std::move(KillValues)) {}
  ~KillMultiple() override = default;
  container_type computeTargets(D Source) override {
    if (KillValues.find(Source) != KillValues.end()) {
      return {};
    }
    return {Source};
  }

protected:
  container_type KillValues;
};

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use killAllFlows() instead")]] KillAll
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  ~KillAll() override = default;
  KillAll(const KillAll &K) = delete;
  KillAll &operator=(const KillAll &K) = delete;
  container_type computeTargets(D /*Source*/) override {
    return container_type();
  }

  static std::shared_ptr<KillAll<D>> getInstance() {
    static std::shared_ptr<KillAll> Instance =
        std::shared_ptr<KillAll>(new KillAll);
    return Instance;
  }

private:
  KillAll() = default;
};

//===----------------------------------------------------------------------===//
// Gen-and-kill flow functions
template <typename D, typename Container = std::set<D>>
class [[deprecated(
    "Use generateFlowAndKillAllOthers() instead")]] GenAndKillAllOthers
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenAndKillAllOthers(D GenValue, D ZeroValue)
      : GenValue(GenValue), ZeroValue(ZeroValue) {}
  ~GenAndKillAllOthers() override = default;
  container_type computeTargets(D Source) override {
    if (Source == ZeroValue) {
      return {ZeroValue, GenValue};
    }
    return {};
  }

private:
  D GenValue;
  D ZeroValue;
};

template <typename D, typename Container = std::set<D>>
class [[deprecated(
    "Use generateManyFlowsAndKillAllOthers() instead")]] GenAllAndKillAllOthers
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  GenAllAndKillAllOthers(const container_type &GenValues, D ZeroValue)
      : GenValues(GenValues), ZeroValue(ZeroValue) {}
  ~GenAllAndKillAllOthers() override = default;
  container_type computeTargets(D Source) override {
    if (Source == ZeroValue) {
      GenValues.insert(Source);
      return GenValues;
    }
    return {};
  }

protected:
  container_type GenValues;
  D ZeroValue;
};

//===----------------------------------------------------------------------===//
// Miscellaneous flow functions

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use transferFlow() instead")]] Transfer
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;

  Transfer(D ToValue, D FromValue) : ToValue(ToValue), FromValue(FromValue) {}
  ~Transfer() override = default;
  container_type computeTargets(D Source) override {
    if (Source == FromValue) {
      return {Source, ToValue};
    }
    if (Source == ToValue) {
      return {};
    }
    return {Source};
  }

protected:
  D ToValue;
  D FromValue;
};

template <typename D, typename Container = std::set<D>>
class [[deprecated("Use unionFlows() instead")]] Union
    : public FlowFunction<D, Container> {
public:
  using typename FlowFunction<D, Container>::container_type;
  using typename FlowFunction<D, Container>::FlowFunctionType;
  using typename FlowFunction<D, Container>::FlowFunctionPtrType;

  Union(const std::vector<FlowFunctionPtrType> &FlowFuncs)
      : FlowFuncs([&FlowFuncs]() {
          if (FlowFuncs.empty()) {
            return std::vector<FlowFunctionPtrType>(
                {Identity<D, Container>::getInstance()});
          }
          return FlowFuncs;
        }()) {}

  ~Union() override = default;
  container_type computeTargets(D Source) override {
    container_type Result;
    for (const auto &FlowFunc : FlowFuncs) {
      container_type Target = FlowFunc->computeTargets(Source);
      Result.insert(Target.begin(), Target.end());
    }
    return Result;
  }

protected:
  const std::vector<FlowFunctionPtrType> FlowFuncs;
};

} // namespace  psr

#endif
