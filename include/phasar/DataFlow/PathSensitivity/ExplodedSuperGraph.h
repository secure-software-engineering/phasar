/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_PATHSENSITIVITY_EXPLODEDSUPERGRAPH_H
#define PHASAR_DATAFLOW_PATHSENSITIVITY_EXPLODEDSUPERGRAPH_H

#include "phasar/DataFlow/IfdsIde/Solver/ESGEdgeKind.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/StableVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <cstdio>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace psr {

/// An explicit representation of the ExplodedSuperGraph (ESG) of an IFDS/IDE
/// analysis.
///
/// Not all covered instructions of a BasicBlock might be present; however, it
/// is guaranteed that for each BasicBlock covered by the analysis there is at
/// least one node in the ExplicitESG containing an instruction from that BB.
template <typename AnalysisDomainTy> class ExplodedSuperGraph {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;

  struct Node {
    static constexpr size_t NoPredId = ~size_t(0);
  };

  struct NodeData {
    d_t Value{};
    n_t Source{};
  };

  struct NodeAdj {
    size_t PredecessorIdx = Node::NoPredId;
    llvm::SmallVector<size_t, 0> Neighbors{};
  };

  class BuildNodeRef;
  class NodeRef {
    friend ExplodedSuperGraph;
    friend class BuildNodeRef;

  public:
    NodeRef() noexcept = default;
    NodeRef(std::nullptr_t) noexcept {}

    [[nodiscard]] ByConstRef<d_t> value() const noexcept {
      assert(*this);
      return Owner->NodeDataOwner[NodeId].Value;
    }

    [[nodiscard]] ByConstRef<n_t> source() const noexcept {
      assert(*this);
      return Owner->NodeDataOwner[NodeId].Source;
    }

    [[nodiscard]] NodeRef predecessor() const noexcept {
      assert(*this);
      auto PredId = Owner->NodeAdjOwner[NodeId].PredecessorIdx;
      return PredId == Node::NoPredId ? NodeRef() : NodeRef(PredId, Owner);
    }

    [[nodiscard]] bool hasNeighbors() const noexcept {
      assert(*this);
      return !Owner->NodeAdjOwner[NodeId].Neighbors.empty();
    }

    [[nodiscard]] bool getNumNeighbors() const noexcept {
      assert(*this);
      return Owner->NodeAdjOwner[NodeId].Neighbors.size();
    }

    [[nodiscard]] auto neighbors() const noexcept {
      assert(*this);

      return llvm::map_range(Owner->NodeAdjOwner[NodeId].Neighbors,
                             [Owner{Owner}](size_t NBIdx) {
                               assert(NBIdx != Node::NoPredId);
                               return NodeRef(NBIdx, Owner);
                             });
    }

    [[nodiscard]] size_t id() const noexcept { return NodeId; }

    explicit operator bool() const noexcept {
      return Owner != nullptr && NodeId != Node::NoPredId;
    }

    [[nodiscard]] friend bool operator==(NodeRef L, NodeRef R) noexcept {
      return L.NodeId == R.NodeId && L.Owner == R.Owner;
    }
    [[nodiscard]] friend bool operator!=(NodeRef L, NodeRef R) noexcept {
      return !(L == R);
    }
    [[nodiscard]] friend bool operator==(NodeRef L,
                                         std::nullptr_t /*R*/) noexcept {
      return L.Owner == nullptr;
    }
    [[nodiscard]] friend bool operator!=(NodeRef L, std::nullptr_t R) noexcept {
      return !(L == R);
    }

    friend llvm::hash_code hash_value(NodeRef NR) noexcept { // NOLINT
      return llvm::hash_combine(NR.NodeId, NR.Owner);
    }

  private:
    explicit NodeRef(size_t NodeId, const ExplodedSuperGraph *Owner) noexcept
        : NodeId(NodeId), Owner(Owner) {}

    size_t NodeId = Node::NoPredId;
    const ExplodedSuperGraph *Owner{};
  };

  class BuildNodeRef {
  public:
    [[nodiscard]] NodeRef operator()(size_t NodeId) const noexcept {
      return NodeRef(NodeId, Owner);
    }

  private:
    explicit BuildNodeRef(const ExplodedSuperGraph *Owner) noexcept
        : Owner(Owner) {}

    const ExplodedSuperGraph *Owner{};
  };

  explicit ExplodedSuperGraph(d_t ZeroValue) noexcept(
      std::is_nothrow_move_constructible_v<d_t>)
      : ZeroValue(std::move(ZeroValue)) {}

  explicit ExplodedSuperGraph(const ExplodedSuperGraph &) = default;
  ExplodedSuperGraph &operator=(const ExplodedSuperGraph &) = delete;

  ExplodedSuperGraph(ExplodedSuperGraph &&) noexcept = default;
  ExplodedSuperGraph &operator=(ExplodedSuperGraph &&) noexcept = default;

  ~ExplodedSuperGraph() = default;

  [[nodiscard]] NodeRef getNodeOrNull(n_t Inst, d_t Fact) const {
    auto It = FlowFactVertexMap.find(
        std::make_pair(std::move(Inst), std::move(Fact)));
    if (It != FlowFactVertexMap.end()) {
      return NodeRef(It->second, this);
    }
    return nullptr;
  }

  [[nodiscard]] NodeRef fromNodeId(size_t NodeId) const noexcept {
    assert(NodeDataOwner.size() == NodeAdjOwner.size());
    assert(NodeId < NodeDataOwner.size());

    return NodeRef(NodeId, this);
  }

  [[nodiscard]] ByConstRef<d_t> getZeroValue() const noexcept {
    return ZeroValue;
  }

  template <typename Container>
  void saveEdges(n_t Curr, d_t CurrNode, n_t Succ, const Container &SuccNodes,
                 ESGEdgeKind Kind) {
    auto PredId = getNodeIdOrNull(Curr, std::move(CurrNode));

    /// The Identity CTR-flow on the zero-value has no meaning at all regarding
    /// path sensitivity, so skip it
    bool MaySkipEdge = Kind == ESGEdgeKind::CallToRet && CurrNode == ZeroValue;
    for (const d_t &SuccNode : SuccNodes) {
      saveEdge(PredId, Curr, CurrNode, Succ, SuccNode, MaySkipEdge);
    }
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] auto node_begin() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return llvm::map_iterator(
        llvm::seq(size_t(0), NodeDataOwner.size()).begin(), BuildNodeRef(this));
  }
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] auto node_end() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return llvm::map_iterator(llvm::seq(size_t(0), NodeDataOwner.size()).end(),
                              BuildNodeRef(this));
  }
  [[nodiscard]] auto nodes() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return llvm::map_range(llvm::seq(size_t(0), NodeDataOwner.size()),
                           BuildNodeRef(this));
  }

  [[nodiscard]] size_t size() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return NodeDataOwner.size();
  }

  /// Printing:

  void printAsDot(llvm::raw_ostream &OS) const {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    OS << "digraph ESG{\n";
    psr::scope_exit ClosingBrace = [&OS] { OS << '}'; };

    for (size_t I = 0, End = NodeDataOwner.size(); I != End; ++I) {
      auto Nod = NodeRef(I, this);
      OS << I << "[label=\"";
      OS.write_escaped(DToString(Nod.value())) << "\"];\n";

      OS << I << "->" << intptr_t(Nod.predecessor().id())
         << R"([style="bold" label=")";
      OS.write_escaped(NToString(Nod.source())) << "\"];\n";
      for (auto NB : Nod.neighbors()) {
        OS << I << "->" << NB.id() << "[color=\"red\"];\n";
      }
    }
  }

  void printAsDot(std::ostream &OS) const {
    llvm::raw_os_ostream ROS(OS);
    printAsDot(ROS);
  }

  void printESGNodes(llvm::raw_ostream &OS) const {
    for (const auto &[Node, _] : FlowFactVertexMap) {
      OS << "( " << NToString(Node.first) << "; " << DToString(Node.second)
         << " )\n";
    }
  }

private:
  struct PathInfoHash {
    size_t operator()(const std::pair<n_t, d_t> &ND) const {
      return std::hash<n_t>()(ND.first) * 31 + std::hash<d_t>()(ND.second);
    }
  };

  struct PathInfoEq {
    bool operator()(const std::pair<n_t, d_t> &Lhs,
                    const std::pair<n_t, d_t> &Rhs) const {
      return Lhs.first == Rhs.first && Lhs.second == Rhs.second;
    }
  };

  [[nodiscard]] std::optional<size_t> getNodeIdOrNull(n_t Inst,
                                                      d_t Fact) const {
    auto It = FlowFactVertexMap.find(
        std::make_pair(std::move(Inst), std::move(Fact)));
    if (It != FlowFactVertexMap.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  void saveEdge(std::optional<size_t> PredId, n_t Curr, d_t CurrNode, n_t Succ,
                d_t SuccNode, bool MaySkipEdge) {
    auto [SuccVtxIt, Inserted] = FlowFactVertexMap.try_emplace(
        std::make_pair(Succ, SuccNode), Node::NoPredId);

    // Save a reference into the FlowFactVertexMap before the SuccVtxIt gets
    // invalidated
    auto &SuccVtxNode = SuccVtxIt->second;

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto makeNode = [this, PredId, Curr, &CurrNode, &SuccNode]() mutable {
      assert(NodeAdjOwner.size() == NodeDataOwner.size());
      auto Ret = NodeDataOwner.size();

      auto &NodData = NodeDataOwner.emplace_back();
      auto &NodAdj = NodeAdjOwner.emplace_back();
      NodData.Value = SuccNode;

      if (!PredId) {
        // For the seeds: Just that the FlowFactVertexMap is filled at that
        // position...
        FlowFactVertexMap[std::make_pair(Curr, CurrNode)] = Ret;
      }

      NodAdj.PredecessorIdx = PredId.value_or(Node::NoPredId);
      NodData.Source = Curr;

      return Ret;
    };

    if (MaySkipEdge && SuccNode == CurrNode) {
      // This CTR edge carries no information, so skip it.
      // We still want to create the destination node for the ret-FF later
      assert(PredId);
      if (Inserted) {
        SuccVtxNode = makeNode();
        NodeAdjOwner.back().PredecessorIdx = Node::NoPredId;
      }
      return;
    }

    if (PredId && NodeDataOwner[*PredId].Value == SuccNode &&
        NodeDataOwner[*PredId].Source->getParent() == Succ->getParent() &&
        SuccNode != ZeroValue) {

      // Identity edge, we don't need a new node; just assign the Pred here
      if (Inserted) {
        SuccVtxNode = *PredId;
        return;
      }

      // This edge has already been here?!
      if (*PredId == SuccVtxNode) {
        return;
      }
    }

    if (Inserted) {
      SuccVtxNode = makeNode();
      return;
    }

    // Node has already been created, but MaySkipEdge above prevented us from
    // connecting with the pred. Now, we have a non-skippable edge to connect to
    NodeRef SuccVtx(SuccVtxNode, this);
    if (!SuccVtx.predecessor()) {
      NodeAdjOwner[SuccVtxNode].PredecessorIdx =
          PredId.value_or(Node::NoPredId);
      NodeDataOwner[SuccVtxNode].Source = Curr;
      return;
    }

    // This node has more than one predecessor; add a neighbor then
    if (SuccVtx.predecessor().id() != PredId.value_or(Node::NoPredId) &&
        llvm::none_of(SuccVtx.neighbors(),
                      [Pred = PredId.value_or(Node::NoPredId)](NodeRef Nd) {
                        return Nd.predecessor().id() == Pred;
                      })) {

      auto NewNode = makeNode();
      NodeAdjOwner[SuccVtxNode].Neighbors.push_back(NewNode);
      return;
    }
  }

  std::vector<NodeData> NodeDataOwner;
  std::vector<NodeAdj> NodeAdjOwner;
  std::unordered_map<std::pair<n_t, d_t>, size_t, PathInfoHash, PathInfoEq>
      FlowFactVertexMap{};

  // ZeroValue
  d_t ZeroValue;
};

} // namespace psr

#endif // PHASAR_DATAFLOW_PATHSENSITIVITY_EXPLODEDSUPERGRAPH_H
