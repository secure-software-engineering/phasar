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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONS_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONS_H_

#include "llvm/Support/Compiler.h"

#include <atomic>
#include <iosfwd>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace psr {
namespace internal {
struct TestEdgeFunction;
} // namespace internal

//
// This class models an edge function for distributive data-flow problems.
//
// An edge function describes a value computation problem along an exploded
// supergraph edge.
//
template <typename L> class EdgeFunction {
public:
  using EdgeFunctionPtrType = std::shared_ptr<EdgeFunction<L>>;

  virtual ~EdgeFunction() = default;

  //
  // This function describes the concrete value computation for its respective
  // exploded supergraph edge. The function(s) will be evaluated once the
  // exploded supergraph has been constructed and the concrete values of the
  // various value computation problems along the supergraph edges are
  // evaluated.
  //
  // Please also refer to the various edge function factories of the
  // EdgeFunctions interface: EdgeFunctions::get*EdgeFunction() for more
  // details.
  //
  [[nodiscard]] virtual L computeTarget(L Source) = 0;

  //
  // This function composes the two edge functions this and SecondFunction. This
  // function is used to extend an edge function in order to construct so-called
  // jump functions that describe the effects of everlonger sequences of code.
  //
  [[nodiscard]] virtual EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType SecondFunction) = 0;

  //
  // This function describes the join of the two edge functions this and
  // OtherFunction. The function is called whenever two edge functions need to
  // be joined, for instance, when two branches lead to a common successor
  // instruction.
  //
  [[nodiscard]] virtual EdgeFunctionPtrType
  joinWith(EdgeFunctionPtrType OtherFunction) = 0;

  [[nodiscard]] virtual bool
      equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType OtherFunction) const = 0;

  virtual void print(std::ostream &OS,
                     [[maybe_unused]] bool IsForDebug = false) const {
    OS << "EdgeFunction";
  }

  [[nodiscard]] std::string str() {
    std::ostringstream OSS;
    print(OSS);
    return OSS.str();
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
  const L TopElement;

public:
  AllTop(const L TopElement) : TopElement(std::move(TopElement)) {}

  ~AllTop() override = default;

  L computeTarget(L /*Source*/) override { return TopElement; }

  EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType /*SecondFunction*/) override {
    return this->shared_from_this();
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override {
    return OtherFunction;
  }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto *Alltop = dynamic_cast<AllTop<L> *>(Other.get())) {
      return (Alltop->TopElement == TopElement);
    }
    return false;
  }

  void print(std::ostream &OS,
             [[maybe_unused]] bool IsForDebug = false) const override {
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
  const L BottomElement;

public:
  AllBottom(const L BottomElement) : BottomElement(std::move(BottomElement)) {}

  ~AllBottom() override = default;

  L computeTarget(L /*Source*/) override { return BottomElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    if (auto *AB = dynamic_cast<AllBottom<L> *>(SecondFunction.get())) {
      return this->shared_from_this();
    }
    if (auto *EI = dynamic_cast<EdgeIdentity<L> *>(SecondFunction.get())) {
      return this->shared_from_this();
    }
    return SecondFunction->composeWith(this->shared_from_this());
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override {
    if (OtherFunction.get() == this ||
        OtherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }
    if (auto *Alltop = dynamic_cast<AllTop<L> *>(OtherFunction.get())) {
      return this->shared_from_this();
    }
    if (auto *EI = dynamic_cast<EdgeIdentity<L> *>(OtherFunction.get())) {
      return this->shared_from_this();
    }
    return this->shared_from_this();
  }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto *AB = dynamic_cast<AllBottom<L> *>(Other.get())) {
      return (AB->BottomElement == BottomElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool /*IsForDebug = false*/) const override {
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
  EdgeIdentity(const EdgeIdentity &EI) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &EI) = delete;

  ~EdgeIdentity() override = default;

  L computeTarget(L Source) override { return Source; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    return SecondFunction;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override {
    if ((OtherFunction.get() == this) ||
        OtherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }
    if (auto *AB = dynamic_cast<AllBottom<L> *>(OtherFunction.get())) {
      return OtherFunction;
    }
    if (auto *AT = dynamic_cast<AllTop<L> *>(OtherFunction.get())) {
      return this->shared_from_this();
    }
    // do not know how to join; hence ask other function to decide on this
    return OtherFunction->joinWith(this->shared_from_this());
  }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    return this == Other.get();
  }

  static EdgeFunctionPtrType getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeFunctionPtrType Instance(new EdgeIdentity<L>());
    return Instance;
  }

  void print(std::ostream &OS, bool /*IsForDebug = false*/) const override {
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

  //
  // Also refer to FlowFunctions::getNormalFlowFunction()
  //
  // Describes a value computation problem along a normal (non-call, non-return)
  // intra-procedural exploded supergraph edge. A normal edge function
  // implementation is queried for each edge that has been generated by appling
  // the flow function returned by FlowFunctions::getNormalFlowFunction(). The
  // supergraph edge whose computation is requested is defined by the supergraph
  // nodes CurrNode and SuccNode.
  //
  // Let instruction_1 := Curr, instruction_2 := Succ, and 0 the tautological
  // lambda fact.
  //
  // The concrete implementation of an edge function e is depending on the
  // analysis problem. In the following, we present a brief, contrived example:
  //
  // Consider the following flow function implementation (cf.
  // FlowFunctions::getNormalFlowfunction()):
  //
  //    f(0) -> {0}       // pass the lambda (or zero fact) as identity
  //    f(o) -> {o, x}    // generate a new fact x from o
  //    f(.) -> {.}       // pass all other facts that hold before instruction_1
  //                      // as identity
  //
  // The above flow-function implementation corresponds to the following edges
  // in the exploded supergraph.
  //
  //                                 0  o      ...
  //                                 |  |\     ...
  // Curr := x = instruction_1 o p   |  | \    ...
  //                                 |  |  |   ...
  //                                 v  v  v   ...
  //                                 0  o  x   ...
  //
  // Succ := y = instruction_2 q r
  //
  // For each edge generated by the respective flow function a normal edge
  // function is queried that describes a value computation. This results in the
  // following queries:
  //
  // getNormalEdgeFunction(0, Curr, 0 Succ);
  // getNormalEdgeFunction(o, Curr, o Succ);
  // getNormalEdgeFunction(o, Curr, x Succ);
  //
  virtual EdgeFunctionPtrType getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                    n_t Succ, d_t SuccNode) = 0;

  //
  // Also refer to FlowFunctions::getCallFlowFunction()
  //
  // Describes a value computation problem along a call flow. A call edge
  // function is queried for each edge that has been generated by applying the
  // flow function that has been returned by FlowFunctions::getCallFlowFunction.
  // The supergraph edge whose computation is requested is defined by the
  // supergraph nodes SrcNode and DestNode.
  //
  // The concrete implementation of an edge function e is depending on the
  // analysis problem. In the following, we present a brief, contrived example:
  //
  // Consider the following flow function implementation (cf.
  // FlowFunctions::getCallFlowFunction()):
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
  //                                        0  o  p   ...
  //                                         \  \  \  ...
  // CallInst := x = CalleeFun(o, p, ...)     \  \  +----------------+
  //                                           \  +----------------  |
  //                                             +-------------+  +  |
  //                                                  ...      |  |  |
  //                                                  ...      |  |  |
  //                                        0  o  p   ...      |  |  |
  //                                                           |  |  |
  //                                                           |  |  |
  //                                                           |  |  |
  //                                 Ty CalleeFun(q, r, ...)   |  |  |
  //                                                           v  v  v
  //                                                           0  q  r   ...
  //
  //                                             start point
  //
  // For each edge generated by the respective flow function a call edge
  // function is queried that describes a value computation. This results in the
  // following queries:
  //
  // getCallEdgeFunction(CallInst, 0, CalleeFun, 0);
  // getCallEdgeFunction(CallInst, o, CalleeFun, q);
  // getCallEdgeFunction(CallInst, p, CalleeFun, r);
  //
  virtual EdgeFunctionPtrType getCallEdgeFunction(n_t CallInst, d_t SrcNode,
                                                  f_t CalleeFun,
                                                  d_t DestNode) = 0;

  //
  // Also refer to FlowFunction::getRetFlowFunction()
  //
  // Describes a value computation problem along a return flow. A return edge
  // function implementation is queried for each edge that has been generated by
  // applying the flow function that has been returned by
  // FlowFunctions::getRetFlowFunction(). The supergraph edge whose computation
  // is requested is defined by the supergraph nodes ExitNode and RetNode.
  //
  // The concrete implementation of an edge function e is depending on the
  // analysis problem. In the following, we present a brief, contrived example:
  //
  // Consider the following flow function implementation (cf.
  // FlowFunctions::getRetFlowFunction()):
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
  //                                               0  o   ...
  //
  // CallSite = RetSite := x = CalleeFun(o, ...)
  //                                                     +------------------+
  //                                                  +--|---------------+  |
  //                                               +--|--|------------+  |  |
  //                                               v  v  v   ...      |  |  |
  //                                               0  o  x   ...      |  |  |
  //                                                                  |  |  |
  //                                                                  |  |  |
  //                                                                  |  |  |
  //                                           Ty CalleeFun(q, ...)   |  |  |
  //                                                                  |  |  |
  //                                                                  0  q  r
  //
  //                                           ExitInst := return r
  //
  // For each edge generated by the respective flow function a return edge
  // function is queried that describes a value computation. This results in the
  // following queries:
  //
  // getReturnEdgeFunction(CallSite, CalleeFun, ExitInst, 0, RetSite, 0);
  // getReturnEdgeFunction(CallSite, CalleeFun, ExitInst, q, RetSite, o);
  // getReturnEdgeFunction(CallSite, CalleeFun, ExitInst, r, RetSite, x);
  //
  virtual EdgeFunctionPtrType getReturnEdgeFunction(n_t CallSite, f_t CalleeFun,
                                                    n_t ExitInst, d_t ExitNode,
                                                    n_t RetSite,
                                                    d_t RetNode) = 0;

  //
  // Also refer to FlowFunctions::getCallToRetFlowFunction()
  //
  // Describes a value computation problem along data-flows alongsite a
  // CallSite. A return edge function implementation is queried for each edge
  // that has been generated by applying the flow function that has been
  // returned by FlowFunctions::getCallToRetFlowFunction(). The supergraph edge
  // whose computation is requested is defined by the supergraph nodes CallNode
  // and RetSiteNode.
  //
  // The concrete implementation of an edge function e is depending on the
  // analysis problem. In the following, we present a brief, contrived example:
  //
  // Consider the following flow function implementation (cf.
  // FlowFunctions::getCallToRetFlowFunction()):
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
  //                                                  0  o   ...
  //                                                  |  |
  //                                                  |  +-------+
  //                                                  +--------+ |
  //                                                           | |
  // CallSite = RetSite := x = CalleeFun(o, p, ...)            | |
  //                                                           | |
  //                                                  +--------+ |
  //                                                  |  +-------+
  //                                                  v  v
  //                                                  0  o  x   ...
  //
  // For each edge generated by the respective flow function a call-to-return
  // edge function is queried that describes a value computation. This results
  // in the following queries:
  //
  // getCallToRetEdgeFunction(CallSite, 0, RetSite, 0, {CalleeFun});
  // getCallToRetEdgeFunction(CallSite, o, RetSite, o, {CalleeFun});
  //
  virtual EdgeFunctionPtrType
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode, std::set<f_t> Callees) = 0;

  //
  // Also refer to FlowFunction::getSummaryFlowFunction()
  //
  // Describes a value computation problem along a summary data flow. A summary
  // edge function implementation is queried for each edge that has been
  // generated by FlowFunctions::getSummaryFlowFunction(). The supergraph edge
  // whose computation is requested is defined by the supergraph nodes CurrNode
  // and SuccNode.
  //
  // The default implementation returns a nullptr to indicate that the mechanism
  // should not be used.
  //
  virtual EdgeFunctionPtrType
  getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ, d_t SuccNode) = 0;
};

// This class can be used as a singleton factory for EdgeFunctions that only
// have constant data.
//
// EdgeFunction that have constant data only need to be allocated once and can
// be turned into a singleton, which save large amounts of memory. The
// automatic singleton creation can be enabled for a EdgeFunction class by
// inheriting from EdgeFunctionSingletonFactory and only allocating
// EdgeFunction throught the provided createEdgeFunction method.
//
// Old unused EdgeFunction that were deallocated need to/should be clean from
// the factory by either calling cleanExpiredEdgeFunctions or initializing the
// automatic cleaner thread with initEdgeFunctionCleaner.
template <typename EdgeFunctionType, typename CtorArgT>
class EdgeFunctionSingletonFactory {
public:
  EdgeFunctionSingletonFactory() = default;
  EdgeFunctionSingletonFactory(const EdgeFunctionSingletonFactory &) = default;
  EdgeFunctionSingletonFactory &
  operator=(const EdgeFunctionSingletonFactory &) = default;
  EdgeFunctionSingletonFactory(EdgeFunctionSingletonFactory &&) noexcept =
      default;
  EdgeFunctionSingletonFactory &
  operator=(EdgeFunctionSingletonFactory &&) noexcept = default;

  virtual ~EdgeFunctionSingletonFactory() {
    std::lock_guard<std::mutex> DataLock(getCacheData().DataMutex);
  }

  // Creates a new EdgeFunction of type EdgeFunctionType, reusing the previous
  // allocation if an EdgeFunction with the same values was already created.
  static inline std::shared_ptr<EdgeFunctionType>
  createEdgeFunction(CtorArgT K) {
    std::lock_guard<std::mutex> DataLock(getCacheData().DataMutex);

    auto &Storage = getCacheData().Storage;
    auto SearchVal = Storage.find(K);
    if (SearchVal != Storage.end() && !SearchVal->second.expired()) {
      return SearchVal->second.lock();
    }
    auto NewEdgeFunc = std::make_shared<EdgeFunctionType>(K);
    Storage[K] = NewEdgeFunc;
    return NewEdgeFunc;
  }

  // Initialize a cleaner thread to automatically remove unused/expired
  // EdgeFunctions. The thread is only started the first time this function is
  // called.
  static inline void initEdgeFunctionCleaner() { getCleaner(); }

  static inline void stopEdgeFunctionCleaner() { getCleaner().stop(); }

  // Clean all unused/expired EdgeFunctions from the internal storage.
  static inline void cleanExpiredEdgeFunctions() {
    cleanExpiredMapEntries(getCacheData());
  }

  LLVM_DUMP_METHOD
  static void dump(bool PrintElements = false) {
    std::lock_guard<std::mutex> DataLock(getCacheData().DataMutex);

    std::cout << "Elements in cache: " << getCacheData().Storage.size();

    if (PrintElements) {
      std::cout << "\n";
      for (auto &KVPair : getCacheData().Storage) {
        std::cout << "(" << KVPair.first << ") -> " << std::boolalpha
                  << KVPair.second.expired() << std::endl;
      }
    }
    std::cout << std::endl;
  }

private:
  struct EFStorageData {
    std::map<CtorArgT, std::weak_ptr<EdgeFunctionType>> Storage{};
    std::mutex DataMutex;
  };

  static inline EFStorageData &getCacheData() {
    static EFStorageData StoredData{};
    return StoredData;
  }

  static void cleanExpiredMapEntries(EFStorageData &CData) {
    std::lock_guard<std::mutex> DataLock(CData.DataMutex);

    auto &Storage = CData.Storage;
    for (auto Iter = Storage.begin(); Iter != Storage.end();) {
      if (Iter->second.expired()) {
        Iter = Storage.erase(Iter);
      } else {
        ++Iter;
      }
    }
  }

  class EdgeFactoryStorageCleaner {
  public:
    EdgeFactoryStorageCleaner()
        : CleanerThread(&EdgeFactoryStorageCleaner::runCleaner, this,
                        std::reference_wrapper<EFStorageData>(
                            EdgeFunctionSingletonFactory::getCacheData())) {
#ifdef __linux__
      pthread_setname_np(CleanerThread.native_handle(),
                         "EdgeFactoryStorageCleanerThread");
#endif
    }
    EdgeFactoryStorageCleaner(const EdgeFactoryStorageCleaner &) = delete;
    EdgeFactoryStorageCleaner &
    operator=(const EdgeFactoryStorageCleaner &) = delete;
    EdgeFactoryStorageCleaner(EdgeFactoryStorageCleaner &&) = delete;
    EdgeFactoryStorageCleaner &operator=(EdgeFactoryStorageCleaner &&) = delete;
    ~EdgeFactoryStorageCleaner() { stop(); }

    // Stops the cleaning thread running in the background.
    void stop() {
      KeepRunning = false;
      if (CleanerThread.joinable()) {
        CleanerThread.join();
      }
    }

  private:
    void runCleaner(EFStorageData &CData) {
      while (KeepRunning) {
        std::this_thread::sleep_for(CleaningPauseTime);
        cleanExpiredMapEntries(CData);
      }
    }

    static constexpr auto CleaningPauseTime = std::chrono::seconds(2);

    std::atomic_bool KeepRunning{true};
    std::thread CleanerThread;
  };

  static inline EdgeFactoryStorageCleaner &getCleaner() {
    static EdgeFactoryStorageCleaner EdgeFunctionCleaner{};
    return EdgeFunctionCleaner;
  }

  friend internal::TestEdgeFunction;
};

} // namespace psr

#endif
