#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CFG.h"
#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/Utilities.h"

#include <limits>
#include <type_traits>

#include <llvm-16/llvm/ADT/STLExtras.h>

namespace psr {

template <typename N, typename F, typename FunctionOfFn>
struct CompressedRevCG {
  const CallGraph<N, F> *CG;
  const Compressor<F, FunctionId> *Functions;
  [[no_unique_address]] FunctionOfFn FunctionOf;
};

template <typename N, typename F, typename FunctionOfFn>
struct GraphTraits<CompressedRevCG<N, F, FunctionOfFn>> {
  using graph_type = CompressedRevCG<N, F, FunctionOfFn>;
  using value_type = F;
  using vertex_t = FunctionId;
  using edge_t = vertex_t;
  static constexpr vertex_t Invalid =
      vertex_t(std::numeric_limits<std::underlying_type_t<vertex_t>>::max());

  static auto transformer(const graph_type &G) {
    return [&G](ByConstRef<N> CS) {
      return G.Functions->getOrNull(G.FunctionOf(CS)).value_or(Invalid);
    };
  }

  static constexpr auto ValidId = [](vertex_t Vtx) { return Vtx != Invalid; };

  static auto outEdges(const graph_type &G, vertex_t Vtx) {
    return llvm::make_filter_range(
        llvm::map_range(G.CG->getCallersOf((*G.Functions)[Vtx]),
                        transformer(G)),
        ValidId);
  }
  static size_t outDegree(const graph_type &G, vertex_t Vtx) {
    return G.CG->getCallersOf((*G.Functions)[Vtx]).size();
  }
  static auto vertices(const graph_type &G) noexcept {
    return psr::iota<vertex_t>(G.Functions->size());
  }

  static size_t size(const graph_type &G) noexcept {
    return G.Functions->size();
  }

  static vertex_t target(edge_t Edge) noexcept { return Edge; }
};

namespace detail {
constexpr auto getCFGFunctionOf(const CFG auto &CF) {
  return [&CF](const auto &Inst) { return CF.getFunctionOf(Inst); };
}
} // namespace detail

template <typename N, typename F>
SCCHolder<FunctionId>
computeCGSCCs(const psr::CallGraph<N, F> &CG, const CFGOf<N, F> auto &CF,
              const Compressor<F, FunctionId> &Functions) {
  return computeSCCs(CompressedRevCG{
      .CG = &CG,
      .Functions = &Functions,
      .FunctionOf = detail::getCFGFunctionOf(CF),
  });
}

template <typename N, typename F>
SCCDependencyGraph<FunctionId>
computeCGSCCCallers(const psr::CallGraph<N, F> &CG, const CFGOf<N, F> auto &CF,
                    const Compressor<F, FunctionId> &Functions,
                    const SCCHolder<FunctionId> &SCCs) {
  return computeSCCDependencies(
      CompressedRevCG{
          .CG = &CG,
          .Functions = &Functions,
          .FunctionOf = detail::getCFGFunctionOf(CF),
      },
      SCCs);
}

} // namespace psr
