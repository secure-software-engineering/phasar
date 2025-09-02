/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_UTILS_SCCGENERIC_H
#define PHASAR_UTILS_SCCGENERIC_H

#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/RepeatIterator.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace psr {
class LLVMBasedICFG;
} // namespace psr

namespace psr {

namespace detail {
struct SCCIdBase {
  uint32_t Value{};

  constexpr SCCIdBase() noexcept = default;

  explicit constexpr SCCIdBase(uint32_t Val) noexcept : Value(Val) {}

  explicit constexpr operator uint32_t() const noexcept { return Value; }
  template <typename T = size_t,
            typename = std::enable_if_t<!std::is_same_v<uint32_t, T>>>
  explicit constexpr operator size_t() const noexcept {
    return Value;
  }

  constexpr uint32_t operator+() const noexcept { return Value; }

  friend constexpr bool operator==(SCCIdBase L, SCCIdBase R) noexcept {
    return L.Value == R.Value;
  }
  friend constexpr bool operator!=(SCCIdBase L, SCCIdBase R) noexcept {
    return !(L == R);
  }
};
} // namespace detail

template <typename GraphNodeId> struct SCCId : detail::SCCIdBase {
  using detail::SCCIdBase::SCCIdBase;
};

} // namespace psr

namespace llvm {
template <typename GraphNodeId> struct DenseMapInfo<psr::SCCId<GraphNodeId>> {
  using SCCId = psr::SCCId<GraphNodeId>;

  static constexpr SCCId getEmptyKey() noexcept { return SCCId(UINT32_MAX); }
  static constexpr SCCId getTombstoneKey() noexcept {
    return SCCId(UINT32_MAX - 1);
  }

  static auto getHashValue(SCCId SCC) noexcept {
    return llvm::hash_value(uint32_t(SCC));
  }
  static constexpr bool isEqual(SCCId SCC1, SCCId SCC2) noexcept {
    return SCC1 == SCC2;
  }
};
} // namespace llvm

namespace psr {

// holds the scc's of a given graph
template <typename GraphNodeId> struct SCCHolder {
  TypedVector<GraphNodeId, SCCId<GraphNodeId>, 0> SCCOfNode;
  TypedVector<SCCId<GraphNodeId>, llvm::SmallVector<GraphNodeId, 1>>
      NodesInSCC{};

  [[nodiscard]] size_t size() const noexcept { return NodesInSCC.size(); }
  [[nodiscard]] bool empty() const noexcept { return NodesInSCC.empty(); }
};

// holds a graph were the scc's are collapsed to a single node. Resulting graph
// is a DAG
template <typename GraphNodeId> struct SCCDependencyGraph {
  TypedVector<SCCId<GraphNodeId>, llvm::SmallDenseSet<SCCId<GraphNodeId>>>
      ChildrenOfSCC;
  llvm::SmallVector<SCCId<GraphNodeId>, 0> SCCRoots;
};

template <typename GraphNodeId>
struct GraphTraits<SCCDependencyGraph<GraphNodeId>> {
  using graph_type = SCCDependencyGraph<GraphNodeId>;
  using value_type = EmptyType;
  using vertex_t = SCCId<GraphNodeId>;
  using edge_t = vertex_t;

  static inline constexpr auto Invalid = vertex_t(UINT32_MAX);

  [[nodiscard]] static const auto &outEdges(const graph_type &G,
                                            vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return G.ChildrenOfSCC[Vtx];
  }

  [[nodiscard]] static size_t outDegree(const graph_type &G,
                                        vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return G.ChildrenOfSCC[Vtx].size();
  }

  [[nodiscard]] static RepeatRangeType<value_type>
  nodes(const graph_type &G) noexcept {
    return repeat(EmptyType{}, G.ChildrenOfSCC.size());
  }

  [[nodiscard]] static llvm::ArrayRef<vertex_t>
  roots(const graph_type &G) noexcept {
    return G.SCCRoots;
  }

  [[nodiscard]] static auto vertices(const graph_type &G) noexcept {
    return iota<vertex_t>(G.Adj.size());
  }

  [[nodiscard]] static value_type node([[maybe_unused]] const graph_type &G,
                                       [[maybe_unused]] vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return {};
  }

  [[nodiscard]] static size_t size(const graph_type &G) noexcept {
    return G.ChildrenOfSCC.size();
  }

  [[nodiscard]] static size_t
  roots_size(const graph_type &G) noexcept { // NOLINT
    return G.SCCRoots.size();
  }

  [[nodiscard]] constexpr vertex_t target(edge_t Edge) noexcept { return Edge; }

  [[nodiscard]] vertex_t withEdgeTarget(edge_t /*edge*/,
                                        vertex_t Tar) noexcept {
    return Tar;
  }
};

// holds topologically sorted SCCDependencyGraph
template <typename GraphNodeId> struct SCCOrder {
  llvm::SmallVector<SCCId<GraphNodeId>, 0> SCCIds;
};

namespace detail {

template <typename GraphNodeId> struct SCCData {
  TypedVector<GraphNodeId, uint32_t, 128> Disc;
  TypedVector<GraphNodeId, uint32_t, 128> Low;
  BitSet<GraphNodeId> OnStack;
  llvm::SmallVector<GraphNodeId> Stack;
  uint32_t Time = 0;
  BitSet<GraphNodeId> Seen;

  explicit SCCData(size_t NumFuns)
      : Disc(NumFuns, UINT32_MAX), Low(NumFuns, UINT32_MAX), OnStack(NumFuns),
        Seen(NumFuns) {}
};

template <typename GraphNodeId> struct SCCDataIt {
  TypedVector<GraphNodeId, uint32_t, 128> Disc;
  TypedVector<GraphNodeId, uint32_t, 128> Low;
  BitSet<GraphNodeId> OnStack;
  llvm::SmallVector<GraphNodeId> Stack;
  llvm::SmallVector<std::pair<GraphNodeId, uint32_t>> CallStack;
  uint32_t Time = 0;
  BitSet<GraphNodeId> Seen;

  explicit SCCDataIt(size_t NumFuns)
      : Disc(NumFuns, UINT32_MAX), Low(NumFuns, UINT32_MAX), OnStack(NumFuns),
        Seen(NumFuns) {}
};

constexpr void setMin(uint32_t &InOut, uint32_t Other) {
  if (Other < InOut) {
    InOut = Other;
  }
}

template <typename G>
static void
computeSCCsRec(const G &Graph, typename GraphTraits<G>::vertex_t CurrNode,
               SCCData<typename GraphTraits<G>::vertex_t> &Data,
               SCCHolder<typename GraphTraits<G>::vertex_t> &Holder) {
  // See
  // https://www.geeksforgeeks.org/tarjan-algorithm-find-strongly-connected-components

  auto CurrTime = Data.Time++;
  Data.Disc[CurrNode] = CurrTime;
  Data.Low[CurrNode] = CurrTime;
  Data.Stack.push_back(CurrNode);
  Data.OnStack.insert(CurrNode);

  using GTraits = psr::GraphTraits<G>;
  using detail::setMin;
  using SCCId = psr::SCCId<typename GraphTraits<G>::vertex_t>;

  for (const auto &OutEdge : GTraits::outEdges(Graph, CurrNode)) {
    auto SuccNode = GTraits::target(OutEdge);
    if (Data.Disc[SuccNode] == UINT32_MAX) {
      // Tree-edge: Not seen yet --> recurse

      computeSCCsRec(Graph, SuccNode, Data, Holder);
      setMin(Data.Low[CurrNode], Data.Low[SuccNode]);
    } else if (Data.OnStack.contains(SuccNode)) {
      // Back-edge --> circle!

      setMin(Data.Low[CurrNode], Data.Disc[SuccNode]);
    }
  }

  if (Data.Low[CurrNode] == Data.Disc[CurrNode]) {
    // Found SCC

    auto SCCIdx = SCCId(Holder.NodesInSCC.size());
    auto &NodesInSCC = Holder.NodesInSCC.emplace_back();

    assert(!Data.Stack.empty());

    while (Data.Stack.back() != CurrNode) {
      auto Fun = Data.Stack.pop_back_val();
      Holder.SCCOfNode[Fun] = SCCIdx;
      Data.OnStack.erase(Fun);
      Data.Seen.insert(Fun);
      NodesInSCC.push_back(Fun);
    }

    auto Fun = Data.Stack.pop_back_val();
    Holder.SCCOfNode[Fun] = SCCIdx;
    Data.OnStack.erase(Fun);
    Data.Seen.insert(Fun);
    NodesInSCC.push_back(Fun);
  }
}

} // namespace detail

template <typename G>
[[nodiscard]] SCCHolder<typename GraphTraits<G>::vertex_t>
computeSCCs(const G &Graph) {
  using GTraits = psr::GraphTraits<G>;

  SCCHolder<typename GTraits::vertex_t> Ret{};

  auto NumNodes = GTraits::size(Graph);
  Ret.SCCOfNode.resize(NumNodes);

  if (!NumNodes) {
    return Ret;
  }

  detail::SCCData<typename GTraits::vertex_t> Data(NumNodes);
  for (auto VtxId : GTraits::vertices(Graph)) {
    if (!Data.Seen.contains(VtxId)) {
      computeSCCsRec(Graph, VtxId, Data, Ret);
    }
  }

  return Ret;
}

// Note: generated by FhGenie GPT o3 Mini
template <typename G>
SCCHolder<typename GraphTraits<std::decay_t<G>>::vertex_t>
computeSCCIterative(const G &Graph) {
  using GTraits = GraphTraits<std::decay_t<G>>;
  using VertexTy = typename GTraits::vertex_t;
  using EdgeTy = typename GTraits::edge_t;
  using SCCId = psr::SCCId<VertexTy>;
  const int UNVISITED = -1;

  // Number of nodes (vertices are assumed to be consecutive indices).
  size_t NumNodes = GTraits::size(Graph);

  // Use TypedVector for per-vertex data instead of unordered_map.
  TypedVector<VertexTy, int> Dfn; // discovery index.
  Dfn.resize(NumNodes, UNVISITED);
  TypedVector<VertexTy, int> Lowlink; // smallest index reachable.
  Lowlink.resize(NumNodes, 0);
  TypedVector<VertexTy, bool> InStack; // marker for Tarjan's stack.
  InStack.resize(NumNodes, false);

  int CurrentIndex = 0;

  // Our final SCC holder. Pre-resize SCCOfNode to the number of nodes.
  SCCHolder<VertexTy> Holder;
  Holder.SCCOfNode.resize(NumNodes);
  // Initially, holder.NodesInSCC is empty and holder.NumSCCs is zero.

  // Instead of storing a vector of out-edges, we store an iterator pair.
  using OutEdgeRange =
      decltype(GTraits::outEdges(Graph, std::declval<VertexTy>()));
  using OutEdgeIterator = decltype(std::begin(std::declval<OutEdgeRange>()));

  // DFS frame holding current vertex and its edge iterator range.
  struct DFSFrame {
    VertexTy V;
    OutEdgeIterator It;
    OutEdgeIterator ItEnd;
  };
  std::vector<DFSFrame> DfsStack;
  std::vector<VertexTy> S; // Tarjan's stack (vertices in the current DFS path).

  // Helper to push a new DFS frame.
  const auto PushFrame = [&](const VertexTy &V) {
    auto &&Range = GTraits::outEdges(Graph, V);
    DFSFrame Frame{
        V,
        std::begin(Range),
        std::end(Range),
    };
    DfsStack.push_back(Frame);
  };

  // Iterate over all vertices (assumed to be dense).
  for (const auto &V : GTraits::vertices(Graph)) {
    if (Dfn[V] != UNVISITED) {
      continue; // already visited
    }

    PushFrame(V);
    Dfn[V] = CurrentIndex;
    Lowlink[V] = CurrentIndex;
    CurrentIndex++;
    S.push_back(V);
    InStack[V] = true;

    // DFS simulation using the explicit stack.
    while (!DfsStack.empty()) {
      DFSFrame &Frame = DfsStack.back();
      VertexTy U = Frame.V;
      if (Frame.It != Frame.ItEnd) {
        // Process the next outgoing edge.
        const EdgeTy &Edge = *(Frame.It++);
        VertexTy W = GTraits::target(Edge);
        if (Dfn[W] == UNVISITED) {
          // w is newly discovered.
          PushFrame(W);
          Dfn[W] = CurrentIndex;
          Lowlink[W] = CurrentIndex;
          CurrentIndex++;
          S.push_back(W);
          InStack[W] = true;
        } else if (InStack[W]) {
          // w is in the current DFS path; update lowlink.
          Lowlink[U] = std::min(Lowlink[U], Dfn[W]);
        }
      } else {
        // Done exploring u.
        if (Lowlink[U] == Dfn[U]) {
          // u is the root of an SCC; pop from S until u is reached.
          auto &Comp = Holder.NodesInSCC.emplace_back(); // The new SCC.
          VertexTy W;
          do {
            W = S.back();
            S.pop_back();
            InStack[W] = false;
            // Assign w the current SCC id.
            Holder.SCCOfNode[W] = static_cast<SCCId>(Holder.size());
            Comp.push_back(W);
          } while (W != U);
        }
        DfsStack.pop_back();
        if (!DfsStack.empty()) {
          // After returning, update the parent's lowlink.
          VertexTy Parent = DfsStack.back().V;
          Lowlink[Parent] = std::min(Lowlink[Parent], Lowlink[U]);
        }
      }
    }
  }

  return Holder;
}

template <typename G>
SCCDependencyGraph<typename GraphTraits<G>::vertex_t> computeSCCDependencies(
    const G &Graph, const SCCHolder<typename GraphTraits<G>::vertex_t> &SCCs) {

  using GTraits = GraphTraits<G>;
  using GraphNodeId = typename GraphTraits<G>::vertex_t;

  SCCDependencyGraph<GraphNodeId> Ret;
  Ret.ChildrenOfSCC.resize(SCCs.size());

  BitSet<SCCId<GraphNodeId>> Roots(SCCs.size(), true);

  for (auto NodeId : GTraits::vertices(Graph)) {
    auto SrcSCC = SCCs.SCCOfNode[NodeId];

    for (const auto &Edge : GTraits::outEdges(Graph, NodeId)) {
      auto Succ = GTraits::target(Edge);
      auto SuccSCC = SCCs.SCCOfNode[Succ];
      if (SuccSCC != SrcSCC) {
        Ret.ChildrenOfSCC[SrcSCC].insert(SuccSCC);
        Roots.erase(SuccSCC);
      }
    }
  }

  Ret.SCCRoots.reserve(Roots.size());
  for (auto Rt : Roots) {
    Ret.SCCRoots.push_back(Rt);
  }

  return Ret;
}

template <typename GraphNodeId>
[[nodiscard]] SCCOrder<GraphNodeId>
computeSCCOrder(const SCCHolder<GraphNodeId> &SCCs,
                const SCCDependencyGraph<GraphNodeId> &Callers) {
  SCCOrder<GraphNodeId> Ret;
  Ret.SCCIds.reserve(SCCs.size());

  BitSet<SCCId<GraphNodeId>> Seen;
  Seen.reserve(SCCs.size());

  auto Dfs = [&](auto &Dfs, SCCId<GraphNodeId> CurrSCC) -> void {
    Seen.insert(CurrSCC);
    for (auto Caller : Callers.ChildrenOfSCC[CurrSCC]) {
      if (!Seen.contains(Caller)) {
        Dfs(Dfs, Caller);
      }
    }
    Ret.SCCIds.push_back(CurrSCC);
  };

  for (auto Leaf : Callers.SCCRoots) {
    if (!Seen.contains(Leaf)) {
      Dfs(Dfs, Leaf);
    }
  }

  std::reverse(Ret.SCCIds.begin(), Ret.SCCIds.end());

  return Ret;
}
} // namespace psr

#endif
