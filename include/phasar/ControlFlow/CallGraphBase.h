/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_CALLGRAPHBASE_H
#define PHASAR_CONTROLFLOW_CALLGRAPHBASE_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/CRTPUtils.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/STLExtras.h"

#include <concepts>
#include <type_traits>

namespace psr {

template <typename T>
concept IsCallGraph =
    requires(const T &CG, typename T::n_t Inst, typename T::f_t Fun) {
      typename T::n_t;
      typename T::f_t;

      /// Returns an iterable range of all possible callee candidates at the
      /// given call-site induced by the used call-graph.
      ///
      /// NOTE: This function is typically called in a hot part of the analysis
      /// and should therefore be very fast
      {
        CG.getCalleesOfCallAt(Inst)
      } -> psr::is_iterable_over_v<typename T::f_t>;

      /// Returns an iterable range of all possible call-site candidates that
      /// may call the given function induced by the used call-graph.
      { CG.getCallersOf(Fun) } -> psr::is_iterable_over_v<typename T::n_t>;

      /// A range of all functions that are vertices in the call-graph. The
      /// number of vertex functions can be retrieved by
      /// getNumVertexFunctions().
      {
        CG.getAllVertexFunctions()
      } -> psr::is_iterable_over_v<typename T::f_t>;

      /// A range of all call-sites that are vertices in the call-graph. The
      /// number of vertex-callsites can be retrived by getNumVertexCallSites().
      {
        CG.getAllVertexCallSites()
      } -> psr::is_iterable_over_v<typename T::n_t>;

      { CG.getNumVertexFunctions() } -> std::convertible_to<size_t>;
      { CG.getNumVertexCallSites() } -> std::convertible_to<size_t>;

      /// Same as getNumVertexFunctions()
      { CG.size() } noexcept -> std::convertible_to<size_t>;
      { CG.empty() } noexcept -> std::convertible_to<bool>;
    };

template <typename T>
concept IsCallGraphRef = IsCallGraph<std::remove_cvref_t<T>>;

template <typename T> struct CGTraits {
  // using n_t
  // using f_t
};

/// Base class of all CallGraph implementations within phasar (currently only
/// CallGraph<N, F>).
/// Only represents the data, not how to create it.
template <typename Derived> class CallGraphBase : public CRTPBase<Derived> {
  friend Derived;
  using CRTPBase<Derived>::self;

public:
  using n_t = typename CGTraits<Derived>::n_t;
  using f_t = typename CGTraits<Derived>::f_t;

  /// Returns an iterable range of all possible callee candidates at the given
  /// call-site induced by the used call-graph.
  ///
  /// NOTE: This function is typically called in a hot part of the analysis and
  /// should therefore be very fast
  [[nodiscard]] constexpr decltype(auto)
  getCalleesOfCallAt(ByConstRef<n_t> Inst) const
      noexcept(noexcept(this->self().getCalleesOfCallAtImpl(Inst))) {
    static_assert(
        is_iterable_over_v<decltype(self().getCalleesOfCallAtImpl(Inst)), f_t>);
    return self().getCalleesOfCallAtImpl(Inst);
  }

  /// Returns an iterable range of all possible call-site candidates that may
  /// call the given function induced by the used call-graph.
  [[nodiscard]] constexpr decltype(auto)
  getCallersOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(this->self().getCallersOfImpl(Fun)), n_t>);
    return self().getCallersOfImpl(Fun);
  }

  /// A range of all functions that are vertices in the call-graph. The number
  /// of vertex functions can be retrieved by getNumVertexFunctions().
  [[nodiscard]] constexpr decltype(auto)
  getAllVertexFunctions() const noexcept {
    return self().getAllVertexFunctionsImpl();
  }

  /// A range of all call-sites that are vertices in the call-graph. The number
  /// of vertex-callsites can be retrived by getNumVertexCallSites().
  [[nodiscard]] constexpr auto getAllVertexCallSites() const noexcept {
    return self().getAllVertexCallSitesImpl();
  }

  [[nodiscard]] constexpr size_t getNumVertexFunctions() const noexcept {
    return self().getNumVertexFunctionsImpl();
  }
  [[nodiscard]] constexpr size_t getNumVertexCallSites() const noexcept {
    return self().getNumVertexCallSitesImpl();
  }

  /// The number of functions within this call-graph
  [[nodiscard]] constexpr size_t size() const noexcept {
    return getNumVertexFunctions();
  }

  [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
};

/// A view over a call-graph that implements psr::GraphTraits on the
/// getCallersOf() relation.
/// Useful to compute CG-SCCs.
template <typename CallGraphTy, typename DB> class ReverseCGGraph {
  static constexpr bool NeedsMapping = !IdType<typename CallGraphTy::f_t>;

  enum class FunctionIdImpl : uint32_t {};

public:
  using FunctionId = std::conditional_t<NeedsMapping, FunctionIdImpl,
                                        typename CallGraphTy::f_t>;

  constexpr ReverseCGGraph(
      NonNullPtr<const CallGraphTy> CGView, NonNullPtr<const DB> IRDB,
      Compressor<typename CallGraphTy::f_t, FunctionId> FC) noexcept
    requires(NeedsMapping)
      : CGView(CGView), IRDB(IRDB), FC(std::move(FC)) {}

  constexpr ReverseCGGraph(NonNullPtr<const CallGraphTy> CGView,
                           NonNullPtr<const DB> IRDB) noexcept
      : CGView(CGView), IRDB(IRDB) {
    FC.reserve(CGView->getNumVertexFunctions());
    for (const auto &Fun : CGView->getAllVertexFunctions()) {
      FC.insert(Fun);
    }
  }

  NonNullPtr<const CallGraphTy> CGView;
  NonNullPtr<const DB> IRDB;
  [[no_unique_address]] std::conditional_t<
      NeedsMapping, Compressor<typename CallGraphTy::f_t, FunctionId>,
      NoneCompressor> FC{};
};

template <typename CallGraphTy, typename DB>
ReverseCGGraph(NonNullPtr<const CallGraphTy>, NonNullPtr<const DB>)
    -> ReverseCGGraph<CallGraphTy, DB>;
template <typename CallGraphTy, typename DB>
ReverseCGGraph(const CallGraphTy *, const DB *)
    -> ReverseCGGraph<CallGraphTy, DB>;

template <typename CallGraphTy, typename DB>
struct GraphTraits<ReverseCGGraph<CallGraphTy, DB>> {
  using graph_type = ReverseCGGraph<CallGraphTy, DB>;
  using value_type = typename CallGraphTy::f_t;
  using vertex_t = typename graph_type::FunctionId;
  using edge_t = vertex_t;

  static constexpr vertex_t Invalid = vertex_t(UINT32_MAX);

  static constexpr auto mapToFunction(const graph_type &G) {
    return [&G](ByConstRef<typename CallGraphTy::n_t> Inst) {
      const auto &Fun = G.IRDB->getFunctionOf(Inst);
      return G.FC.getOrNull(Fun).value();
    };
  }

  static constexpr auto outEdges(const graph_type &G, vertex_t Vtx) {
    return llvm::map_range(G.CGView->getCallersOf(G.FC[Vtx]), mapToFunction(G));
  }

  static constexpr decltype(auto) nodes(const graph_type &G) {
    return G.CGView->getAllVertexFunctions();
  }

  // TODO: Roots

  static constexpr auto vertices(const graph_type &G) {
    return iota<vertex_t>(G.CGView->getNumVertexFunctions());
  }

  static constexpr decltype(auto) node(const graph_type &G, vertex_t Vtx) {
    return G.FC[Vtx];
  }

  static constexpr size_t size(const graph_type &G) {
    return G.CGView->getNumVertexFunctions();
  }

  static constexpr vertex_t target(edge_t Edge) { return Edge; }
  static constexpr vertex_t withEdgeTarget(edge_t /*Edge*/, vertex_t Vtx) {
    return Vtx;
  }
};

} // namespace psr

#endif // PHASAR_CONTROLFLOW_CALLGRAPHBASE_H
