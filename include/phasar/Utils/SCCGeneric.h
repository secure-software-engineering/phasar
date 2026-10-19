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
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <functional>
#include <type_traits>

namespace psr {

/// \brief Holds the SCCs of a given graph. Each SCC is assigned a unique
/// sequential id.
template <typename GraphNodeId> struct SCCHolder {
  TypedVector<GraphNodeId, SCCId<GraphNodeId>> SCCOfNode;
  TypedVector<SCCId<GraphNodeId>, llvm::SmallVector<GraphNodeId, 1>>
      NodesInSCC{};

  /// Number of SCCs
  [[nodiscard]] size_t size() const noexcept { return NodesInSCC.size(); }
  [[nodiscard]] bool empty() const noexcept { return NodesInSCC.empty(); }

  /// \brief Prints the given Graph as dot, highlighting the SCCs in the graph.
  ///
  /// \param Graph The graph to print
  /// \param OS The output-stream, where to print into
  /// \param Name The name of the graph
  /// \param NodeToString If the graph has node-labels, convert a node-label to
  /// string
  template <is_const_graph G, typename NodeTransform = DefaultNodeTransform>
    requires std::is_same_v<typename GraphTraits<G>::vertex_t, GraphNodeId>
  void print(const G &Graph, llvm::raw_ostream &OS, llvm::StringRef Name = "",
             NodeTransform NodeToString = {}) const {
    OS << "digraph \"";
    OS.write_escaped(Name) << "\" {\n";
    psr::scope_exit CloseBrace = [&] { OS << "}\n"; };

    using GTraits = psr::GraphTraits<G>;

    for (const auto &[SCCId, SCC] : NodesInSCC.enumerate()) {
      OS << "  subgraph cluster_" << +SCCId
         << "{\n    node [style=filled]; color=blue; label=\"SCC " << +SCCId
         << "\";\n";
      psr::scope_exit CloseSCC = [&] { OS << "  }\n"; };

      for (auto Nod : SCC) {
        OS << "    " << size_t(Nod);
        if constexpr (!std::is_empty_v<typename GTraits::value_type>) {
          OS << "[label=\"";
          OS.write_escaped(
              std::invoke(NodeToString, GTraits::node(Graph, Nod)));
          OS << "\"]";
        }
        OS << ";\n";
      }
    }

    for (auto FromVtx : GTraits::vertices(Graph)) {
      for (const auto &Succ : GTraits::outEdges(Graph, FromVtx)) {
        OS << "  " << size_t(FromVtx) << "->";
        if constexpr (is_llvm_printable_v<decltype(Succ)>) {
          // to print the edge-weight as well, if possible
          OS << Succ;
        } else {
          OS << size_t(GTraits::target(Succ));
        }
        OS << ";\n";
      }
    }
  }
};

/// \brief Holds a graph where the SCCs are collapsed to a single node.
/// Conforms to the is_const_graph concept.
template <typename GraphNodeId> struct SCCDependencyGraph {
  TypedVector<SCCId<GraphNodeId>, llvm::SmallDenseSet<SCCId<GraphNodeId>>>
      ChildrenOfSCC;
  llvm::SmallVector<SCCId<GraphNodeId>, 0> SCCRoots;
};

/// \brief Implements the is_const_graph concept for SCCDependencyGraph
template <typename GraphNodeId>
struct GraphTraits<SCCDependencyGraph<GraphNodeId>> {
  using graph_type = SCCDependencyGraph<GraphNodeId>;
  using value_type = EmptyType;
  using vertex_t = SCCId<GraphNodeId>;
  using edge_t = vertex_t;

  static constexpr auto Invalid = vertex_t(UINT32_MAX);

  [[nodiscard]] static constexpr const auto &outEdges(const graph_type &G,
                                                      vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return G.ChildrenOfSCC[Vtx];
  }

  [[nodiscard]] static constexpr size_t outDegree(const graph_type &G,
                                                  vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return G.ChildrenOfSCC[Vtx].size();
  }

  [[nodiscard]] static constexpr auto nodes(const graph_type &G) noexcept {
    return repeat(EmptyType{}, G.ChildrenOfSCC.size());
  }

  [[nodiscard]] static constexpr llvm::ArrayRef<vertex_t>
  roots(const graph_type &G) noexcept {
    return G.SCCRoots;
  }

  [[nodiscard]] static constexpr auto vertices(const graph_type &G) noexcept {
    return iota<vertex_t>(G.ChildrenOfSCC.size());
  }

  [[nodiscard]] static constexpr value_type
  node([[maybe_unused]] const graph_type &G,
       [[maybe_unused]] vertex_t Vtx) noexcept {
    assert(G.ChildrenOfSCC.inbounds(Vtx));
    return {};
  }

  [[nodiscard]] static constexpr size_t size(const graph_type &G) noexcept {
    return G.ChildrenOfSCC.size();
  }

  [[nodiscard]] static constexpr size_t
  roots_size(const graph_type &G) noexcept { // NOLINT
    return G.SCCRoots.size();
  }

  [[nodiscard]] static constexpr vertex_t target(edge_t Edge) noexcept {
    return Edge;
  }

  [[nodiscard]] static constexpr vertex_t
  withEdgeTarget(edge_t /*edge*/, vertex_t Tar) noexcept {
    return Tar;
  }
};

/// \brief Holds topologically sorted SCCDependencyGraph nodes
template <typename GraphNodeId> struct SCCOrder {
  llvm::SmallVector<SCCId<GraphNodeId>, 0> SCCIds;
};

/// \brief Creates a graph based on the given input Graph, collapsing all SCCs
/// to single nodes. The resulting graph is always a DAG, i.e., it contains no
/// cycles
template <typename G>
// TODO: is_const_graph is too restrictive!
// #if __cplusplus >= 202002L
//   requires is_const_graph<G>
// #endif
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
  Roots.foreach ([&](auto Rt) { Ret.SCCRoots.push_back(Rt); });

  return Ret;
}

/// \brief Computes a topological order of the nodes in the given
/// dependency-graph.
///
/// Uses a simple, recursive postorder-DFS search to find a topological
/// ordering.
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

/// State of Pearce's SCC algorithm. Can be reused across multiple calls to
/// visitSCCsFrom() on the same graph and, after reset(), on other graphs to
/// avoid repeated allocations.
template <typename GraphNodeId> struct PearceSCCData {
  TypedVector<GraphNodeId, uint32_t> RIndex; // only per-vertex array
  BitSet<GraphNodeId> Root;                  // root[v] in Algorithm 4
  uint32_t Index = 1;                        // DFS counter
  uint32_t C{};                              // SCC id counter
  llvm::SmallVector<GraphNodeId> Stack;

  PearceSCCData() noexcept = default;
  explicit PearceSCCData(size_t Num) { reset(Num); }

  /// Prepares the state for a fresh search on a graph with Num vertices.
  void reset(size_t Num) {
    RIndex.assign(Num, 0);
    Root.clear();
    Root.reserve(Num);
    Index = 1;
    C = Num ? uint32_t(Num - 1) : 0;
    Stack.clear();
  }

  [[nodiscard]] bool isVisited(GraphNodeId Vtx) const noexcept {
    return RIndex[Vtx] != 0;
  }
};

namespace detail {

/// Completes the SCC rooted at Root. The members are the stack elements with
/// an rindex not smaller than the root's.
template <typename GraphNodeId, typename OnSCCFn>
void pearce4FinishSCC(PearceSCCData<GraphNodeId> &Data, GraphNodeId Root,
                      OnSCCFn &OnSCC) {
  auto RootIndex = Data.RIndex[Root];
  size_t NumMembers = 1;
  while (NumMembers <= Data.Stack.size() &&
         RootIndex <= Data.RIndex[Data.Stack[Data.Stack.size() - NumMembers]]) {
    ++NumMembers;
  }
  Data.Stack.push_back(Root);

  auto Members = llvm::ArrayRef<GraphNodeId>(Data.Stack).take_back(NumMembers);
  auto NewSCC = SCCId<GraphNodeId>(uint32_t(Data.RIndex.size() - 1 - Data.C));
  std::invoke(OnSCC, NewSCC, Members);

  for (auto W : Members) {
    Data.RIndex[W] = Data.C;
  }
  Data.Index -= uint32_t(NumMembers);
  Data.C--;
  Data.Stack.truncate(Data.Stack.size() - NumMembers);
}

// Recursive variant of Pearce's algorithm (based on Algo 3 in the paper)
template <typename G, typename OnDiscoverFn, typename OnSCCFn>
static void
pearce4VisitRec(const G &Graph, typename GraphTraits<G>::vertex_t V,
                PearceSCCData<typename GraphTraits<G>::vertex_t> &Data,
                OnDiscoverFn &OnDiscover, OnSCCFn &OnSCC) {
  using GTraits = psr::GraphTraits<G>;

  bool Root = true;
  Data.RIndex[V] = Data.Index++;
  std::invoke(OnDiscover, V);

  for (const auto &Edge : GTraits::outEdges(Graph, V)) {
    auto W = GTraits::target(Edge);
    if (Data.RIndex[W] == 0) {
      pearce4VisitRec(Graph, W, Data, OnDiscover, OnSCC);
    }
    if (Data.RIndex[W] < Data.RIndex[V]) {
      Data.RIndex[V] = Data.RIndex[W];
      Root = false;
    }
  }

  if (Root) {
    pearce4FinishSCC(Data, V, OnSCC);
  } else {
    Data.Stack.push_back(V);
  }
}

// Iterative variant of Pearce's algorithm (adapted from on Algo 4 in the paper)
template <typename G, typename OnDiscoverFn, typename OnSCCFn>
static void
pearce4VisitIt(const G &Graph, typename GraphTraits<G>::vertex_t Start,
               PearceSCCData<typename GraphTraits<G>::vertex_t> &Data,
               OnDiscoverFn &OnDiscover, OnSCCFn &OnSCC) {
  using GTraits = psr::GraphTraits<G>;
  using Vertex = typename GTraits::vertex_t;

  using OutEdgeRange =
      decltype(GTraits::outEdges(Graph, std::declval<Vertex>()));
  using OutEdgeIterator =
      decltype(llvm::adl_begin(std::declval<OutEdgeRange &>()));
  using OutEdgeSentinel =
      decltype(llvm::adl_end(std::declval<OutEdgeRange &>()));

  struct DfsFrame {
    Vertex CurrVtx;
    OutEdgeIterator It;
    [[no_unique_address]] OutEdgeSentinel End;
  };

  llvm::SmallVector<DfsFrame> CallStack;

  const auto PushFrames = [&](Vertex V, DfsFrame *Frame) {
    if (Frame->It == Frame->End) {
      return false;
    }
    // Recurse into children until reaching the bottom
    do {
      auto W = GTraits::target(*Frame->It);

      if (Data.RIndex[W] != 0) {
        // Already pushed the children of W
        break;
      }

      Data.RIndex[W] = Data.Index++;
      Data.Root.insert(W);
      std::invoke(OnDiscover, W);

      auto &&OutEdges = GTraits::outEdges(Graph, W);
      Frame = &CallStack.emplace_back(
          DfsFrame{W, llvm::adl_begin(OutEdges), llvm::adl_end(OutEdges)});
      V = W;

    } while (Frame->It != Frame->End);

    return true;
  };

  const auto VisitLoop = [&](Vertex V, DfsFrame &Frame) {
    // Finish visiting the current child and advance to the next child
    if (Frame.It != Frame.End) {
      auto W = GTraits::target(*Frame.It);
      if (Data.RIndex[W] < Data.RIndex[V]) {
        Data.RIndex[V] = Data.RIndex[W];
        Data.Root.erase(V);
      }

      ++Frame.It;
    }
  };

  const auto FinishFrame = [&](Vertex V) {
    // finish visiting V and backtrack to the parent

    if (Data.Root.contains(V)) {
      pearce4FinishSCC(Data, V, OnSCC);
    } else {
      Data.Stack.push_back(V);
    }

    CallStack.pop_back();
  };

  // Initialize the callstack by pushing the initial frame
  Data.RIndex[Start] = Data.Index++;
  Data.Root.insert(Start);
  std::invoke(OnDiscover, Start);
  {
    auto &&OutEdges = GTraits::outEdges(Graph, Start);
    static_assert(
        std::is_lvalue_reference_v<decltype(OutEdges)> ||
            std::is_trivially_destructible_v<std::decay_t<decltype(OutEdges)>>,
        "We assume that outEdges gives either a reference or a view into "
        "the out-edges, but never an owning container by value. Otherwise, "
        "the DFSFrame iterators may be dangling");
    CallStack.emplace_back(
        DfsFrame{Start, llvm::adl_begin(OutEdges), llvm::adl_end(OutEdges)});
  }

  // Simulate the recursion

  PushFrames(Start, &CallStack.back());
  while (true) {
    auto &Frame = CallStack.back();
    Vertex V = Frame.CurrVtx;
    VisitLoop(V, Frame);
    if (PushFrames(V, &Frame)) {
      continue; // we don't pop from the callstack here
    }

    FinishFrame(V);
    if (CallStack.empty()) {
      break;
    }
  }
}

} // namespace detail

/// Runs Pearce's SCC algorithm from Start, visiting all vertices reachable
/// from Start that have not yet been visited with the same Data. Call it with
/// multiple start vertices to cover more of the graph.
///
/// \param OnDiscover Called with each vertex when it is visited first, i.e.,
/// in DFS pre-order.
/// \param OnSCC Called with each SCC when it is complete, in reverse
/// topological order. SCC ids are sequential, starting at 0 after
/// Data.reset(). Members.back() is the first discovered member, i.e., the root
/// of the SCC in the DFS tree. At this point, all SCCs reachable from this SCC
/// are complete and the DFS subtree of the root is fully discovered.
template <typename G, bool Iterative = true>
void visitSCCsFrom(
    const G &Graph, typename GraphTraits<G>::vertex_t Start,
    PearceSCCData<typename GraphTraits<G>::vertex_t> &Data,
    std::invocable<typename GraphTraits<G>::vertex_t> auto OnDiscover,
    std::invocable<SCCId<typename GraphTraits<G>::vertex_t>,
                   llvm::ArrayRef<typename GraphTraits<G>::vertex_t>> auto
        OnSCC,
    std::bool_constant<Iterative> /*Iterative*/ = {}) {
  assert(!Data.isVisited(Start));
  if constexpr (Iterative) {
    detail::pearce4VisitIt(Graph, Start, Data, OnDiscover, OnSCC);
  } else {
    detail::pearce4VisitRec(Graph, Start, Data, OnDiscover, OnSCC);
  }
  assert(Data.Stack.empty() && "The start vertex must be an SCC root");
}

/// Compute SCCs adapted from the paper "A Space-Efficient Algorithm for Finding
/// Strongly Connected Components", Pearce 2015, DOI:
/// <https://doi.org/10.1016/j.ipl.2015.08.010>
///
/// \tparam G The graph-type
/// \tparam Iterative Whether to use the iterative or recursive variant of the
/// algorithm (default: true)
/// \param Graph The graph for with to compute SCCs and topological ordering
template <typename G, bool Iterative = true>
[[nodiscard]] SCCHolder<typename GraphTraits<G>::vertex_t>
computeSCCs(const G &Graph, std::bool_constant<Iterative> /*Iterative*/ = {}) {
  using GTraits = psr::GraphTraits<G>;
  using Vertex = typename GTraits::vertex_t;

  SCCHolder<Vertex> Ret;
  auto N = GTraits::size(Graph);
  if (!N) {
    return Ret;
  }

  Ret.SCCOfNode.resize(N);

  PearceSCCData<Vertex> Data(N);

  const auto OnSCC = [&Ret](SCCId<Vertex> SCC, llvm::ArrayRef<Vertex> Members) {
    assert(size_t(SCC) == Ret.NodesInSCC.size());
    Ret.NodesInSCC.emplace_back(Members.begin(), Members.end());
    for (auto Member : Members) {
      Ret.SCCOfNode[Member] = SCC;
    }
  };

  // for all v in V do if rindex[v]==0 then visit(v)
  for (auto V : GTraits::vertices(Graph)) {
    if (!Data.isVisited(V)) {
      visitSCCsFrom(
          Graph, V, Data, [](Vertex /*Vtx*/) {}, OnSCC,
          std::bool_constant<Iterative>{});
    }
  }

  return Ret;
}

/// Compute SCCs and a topological ordering on the SCCs, adapted from the paper
/// "A Space-Efficient Algorithm for Finding Strongly Connected Components",
/// Pearce 2015, DOI: <https://doi.org/10.1016/j.ipl.2015.08.010>
///
/// \tparam G The graph-type \tparam Iterative Whether to use the iterative
/// or recursive variant of the algorithm (default: true) \param Graph The graph
/// for with to compute SCCs and topological ordering
template <typename G, bool Iterative = true>
[[nodiscard]] std::pair<SCCHolder<typename GraphTraits<G>::vertex_t>,
                        SCCOrder<typename GraphTraits<G>::vertex_t>>
computeSCCsAndTopologicalOrder(
    const G &Graph, std::bool_constant<Iterative> /*Iterative*/ = {}) {
  using Vertex = typename GraphTraits<G>::vertex_t;

  std::pair<SCCHolder<Vertex>, SCCOrder<Vertex>> Ret = {
      computeSCCs(Graph, std::bool_constant<Iterative>{}),
      {},
  };

  // Pearce's algorithm produces SCCs in reverse topological order
  auto Ids = llvm::reverse(psr::iota<SCCId<Vertex>>(Ret.first.size()));
  Ret.second.SCCIds.append(Ids.begin(), Ids.end());

  return Ret;
}

} // namespace psr

#endif
