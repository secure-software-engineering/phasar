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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_FLOWFUNCTIONS_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_FLOWFUNCTIONS_H_

#include "llvm/ADT/ArrayRef.h"

#include <functional>
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

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType identityFlow() {
  return Identity<D, Container>::getInstance();
}

template <typename D, typename Fn, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType lambdaFlow(Fn &&F) {
  struct LambdaFlow : public FlowFunction<D, Container> {
    LambdaFlow(Fn &&F) : Flow(std::forward<Fn>(F)) {}
    Container computeTargets(D Source) override { return Flow(Source); }

    std::decay_t<Fn> Flow;
  };

  return std::make_shared<LambdaFlow>(std::forward<Fn>(F));
}

// template <typename D, typename Container = std::set<D>>
// class Compose : public FlowFunction<D, Container> {
// public:
//   using typename FlowFunction<D, Container>::FlowFunctionType;
//   using typename FlowFunction<D, Container>::FlowFunctionPtrType;

//   using typename FlowFunction<D, Container>::container_type;

//   Compose(const std::vector<FlowFunction<D>> &Funcs) : Funcs(Funcs) {}

//   ~Compose() override = default;

//   container_type computeTargets(const D &Source) override {
//     container_type Current(Source);
//     for (const FlowFunctionType &Func : Funcs) {
//       container_type Next;
//       for (const D &Fact : Current) {
//         container_type Target = Func.computeTargets(Fact);
//         Next.insert(Target.begin(), Target.end());
//       }
//       Current = Next;
//     }
//     return Current;
//   }

//   static FlowFunctionPtrType
//   compose(const std::vector<FlowFunctionType> &Funcs) {
//     std::vector<FlowFunctionType> Vec;
//     for (const FlowFunctionType &Func : Funcs) {
//       if (Func != Identity<D, Container>::getInstance()) {
//         Vec.insert(Func);
//       }
//     }
//     if (Vec.size() == 1) { // NOLINT(readability-container-size-empty)
//       return Vec[0];
//     }
//     if (Vec.empty()) {
//       return Identity<D, Container>::getInstance();
//     }
//     return std::make_shared<Compose>(Vec);
//   }

// protected:
//   const std::vector<FlowFunctionType> Funcs;
// };

//===----------------------------------------------------------------------===//
// Gen flow functions

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
generateFlow(D FactToGenerate, D From) {
  struct GenFrom : public FlowFunction<D, Container> {
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

/**
 * @brief Generates the given value if the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be generated.
 */
template <typename D, typename Fn, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
generateFlowIf(D FactToGenerate, Fn Predicate) {
  struct GenFlowIf : public FlowFunction<D, Container> {
    GenFlowIf(D GenValue, Fn &&Predicate)
        : GenValue(std::move(GenValue)),
          Predicate(std::forward<Fn>(Predicate)) {}

    Container computeTargets(D Source) override {
      if (Predicate(Source)) {
        return {std::move(Source), GenValue};
      }
      return {std::move(Source)};
    }

    D GenValue;
    std::decay_t<Fn> Predicate;
  };

  return std::make_shared<GenFlowIf>(std::move(FactToGenerate),
                                     std::forward<Fn>(Predicate));
}

template <typename D, typename Range, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
generateManyFlows(Range &&FactsToGenerate, D From) {
  struct GenMany : public FlowFunction<D, Container> {
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

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
killFlow(D FactToKill) {
  struct KillFlow : public FlowFunction<D, Container> {
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

/// \brief Kills all facts for which the given predicate evaluates to true.
/// \tparam D The type of data-flow facts to be killed.
template <typename D, typename Fn, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
killFlowIf(Fn Predicate) {
  struct KillFlowIf : public FlowFunction<D, Container> {
    KillFlowIf(Fn &&Predicate) : Predicate(std::forward<Fn>(Predicate)) {}

    Container computeTargets(D Source) override {
      if (Predicate(Source)) {
        return {};
      }
      return {std::move(Source)};
    }

    std::decay_t<Fn> Predicate;
  };

  return std::make_shared<KillFlowIf>(std::forward<Fn>(Predicate));
}

template <typename D, typename Range, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
killManyFlows(Range &&FactsToKill) {
  struct KillMany : public FlowFunction<D, Container> {
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

template <typename D, typename Container = std::set<D>>
class KillAll : public FlowFunction<D, Container> {
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

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType killAllFlows() {
  return KillAll<D, Container>::getInstance();
}

//===----------------------------------------------------------------------===//
// Gen-and-kill flow functions

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
generateFlowAndKillAllOthers(D FactToGenerate, D From) {
  struct GenFlowAndKillAllOthers : public FlowFunction<D, Container> {
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

template <typename D, typename Range, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
generateManyFlowsAndKillAllOthers(Range &&FactsToGenerate, D From) {
  struct GenManyAndKillAllOthers : public FlowFunction<D, Container> {
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

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
transferFlow(D FactToGenerate, D From) {
  struct TransferFlow : public FlowFunction<D, Container> {
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

template <typename D, typename Container = std::set<D>>
typename FlowFunction<D, Container>::FlowFunctionPtrType
unionFlows(typename FlowFunction<D, Container>::FlowFunctionPtrType OneFF,
           typename FlowFunction<D, Container>::FlowFunctionPtrType OtherFF) {
  struct UnionFlow : public FlowFunction<D, Container> {
    UnionFlow(typename FlowFunction<D, Container>::FlowFunctionPtrType OneFF,
              typename FlowFunction<D, Container>::FlowFunctionPtrType
                  OtherFF) noexcept
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

    typename FlowFunction<D, Container>::FlowFunctionPtrType OneFF;
    typename FlowFunction<D, Container>::FlowFunctionPtrType OtherFF;
  };

  return std::make_shared<UnionFlow>(std::move(OneFF), std::move(OtherFF));
}

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
  virtual FlowFunctionPtrType getSummaryFlowFunction(n_t Curr,
                                                     f_t CalleeFun) = 0;
};
} // namespace  psr

#endif
