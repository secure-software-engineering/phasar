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
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace psr {

namespace detail {
enum class [[clang::enum_extensibility(open)]] //
ExplodedSuperGraphNodeId : size_t{
    NoPredId = SIZE_MAX,
};
} // namespace detail

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

  using NodeId = detail::ExplodedSuperGraphNodeId;

  struct NodeData {
    d_t Value{};
    n_t Source{};
  };

  struct NodeAdj {
    NodeId PredecessorIdx = NodeId::NoPredId;
    llvm::SmallVector<NodeId, 0> Neighbors{};
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
      return Owner->NodeDataOwner[size_t(Id)].Value;
    }

    [[nodiscard]] ByConstRef<n_t> source() const noexcept {
      assert(*this);
      return Owner->NodeDataOwner[size_t(Id)].Source;
    }

    [[nodiscard]] NodeRef predecessor() const noexcept {
      assert(*this);
      auto PredId = Owner->NodeAdjOwner[size_t(Id)].PredecessorIdx;
      return PredId == NodeId::NoPredId ? NodeRef() : NodeRef(PredId, Owner);
    }

    [[nodiscard]] bool hasNeighbors() const noexcept {
      assert(*this);
      return !Owner->NodeAdjOwner[size_t(Id)].Neighbors.empty();
    }

    [[nodiscard]] bool getNumNeighbors() const noexcept {
      assert(*this);
      return Owner->NodeAdjOwner[size_t(Id)].Neighbors.size();
    }

    [[nodiscard]] auto neighbors() const noexcept {
      assert(*this);

      return llvm::map_range(Owner->NodeAdjOwner[size_t(Id)].Neighbors,
                             [Owner{Owner}](NodeId NBIdx) {
                               assert(NBIdx != NodeId::NoPredId);
                               return NodeRef(NBIdx, Owner);
                             });
    }

    [[nodiscard]] NodeId id() const noexcept { return Id; }

    explicit operator bool() const noexcept {
      return Owner != nullptr && Id != NodeId::NoPredId;
    }

    [[nodiscard]] friend bool operator==(NodeRef L, NodeRef R) noexcept {
      return L.Id == R.Id && L.Owner == R.Owner;
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
      return llvm::hash_combine(NR.Id, NR.Owner);
    }

  private:
    explicit NodeRef(NodeId NodeId, const ExplodedSuperGraph *Owner) noexcept
        : Id(NodeId), Owner(Owner) {}

    NodeId Id = NodeId::NoPredId;
    const ExplodedSuperGraph *Owner{};
  };

  class BuildNodeRef {
  public:
    [[nodiscard]] NodeRef operator()(NodeId Id) const noexcept {
      return NodeRef(Id, Owner);
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

  [[nodiscard]] NodeRef fromNodeId(NodeId Id) const noexcept {
    assert(NodeDataOwner.size() == NodeAdjOwner.size());
    assert(size_t(Id) < NodeDataOwner.size());

    return NodeRef(Id, this);
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
    return llvm::map_iterator(IotaIterator<NodeId>{}, BuildNodeRef(this));
  }
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] auto node_end() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return llvm::map_iterator(
        IotaIterator<NodeId>{NodeId(NodeDataOwner.size())}, BuildNodeRef(this));
  }
  [[nodiscard]] auto nodes() const noexcept {
    assert(NodeAdjOwner.size() == NodeDataOwner.size());
    return llvm::make_range(node_begin(), node_end());
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
      auto Nod = NodeRef(NodeId(I), this);
      OS << I << "[label=\"";
      OS.write_escaped(DToString(Nod.value())) << "\"];\n";

      OS << I << "->" << intptr_t(Nod.predecessor().id())
         << R"([style="bold" label=")";
      OS.write_escaped(NToString(Nod.source())) << "\"];\n";
      for (auto NB : Nod.neighbors()) {
        OS << I << "->" << size_t(NB.id()) << "[color=\"red\"];\n";
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

  [[nodiscard]] std::optional<NodeId> getNodeIdOrNull(n_t Inst,
                                                      d_t Fact) const {
    auto It = FlowFactVertexMap.find(
        std::make_pair(std::move(Inst), std::move(Fact)));
    if (It != FlowFactVertexMap.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  void saveEdge(std::optional<NodeId> PredId, n_t Curr, d_t CurrNode, n_t Succ,
                d_t SuccNode, bool MaySkipEdge) {
    auto [SuccVtxIt, Inserted] = FlowFactVertexMap.try_emplace(
        std::make_pair(Succ, SuccNode), NodeId::NoPredId);

    // Save a reference into the FlowFactVertexMap before the SuccVtxIt gets
    // invalidated
    auto &SuccVtxNode = SuccVtxIt->second;

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto makeNode = [this, PredId, Curr, &CurrNode, &SuccNode]() mutable {
      assert(NodeAdjOwner.size() == NodeDataOwner.size());
      auto Ret = NodeId(NodeDataOwner.size());

      auto &NodData = NodeDataOwner.emplace_back();
      auto &NodAdj = NodeAdjOwner.emplace_back();
      NodData.Value = SuccNode;

      if (!PredId) {
        // For the seeds: Just that the FlowFactVertexMap is filled at that
        // position...
        FlowFactVertexMap[std::make_pair(Curr, CurrNode)] = Ret;
      }

      NodAdj.PredecessorIdx = PredId.value_or(NodeId::NoPredId);
      NodData.Source = Curr;

      return Ret;
    };

    if (MaySkipEdge && SuccNode == CurrNode) {
      // This CTR edge carries no information, so skip it.
      // We still want to create the destination node for the ret-FF later
      assert(PredId);
      if (Inserted) {
        SuccVtxNode = makeNode();
        NodeAdjOwner.back().PredecessorIdx = NodeId::NoPredId;
      }
      return;
    }

    if (PredId && NodeDataOwner[size_t(*PredId)].Value == SuccNode &&
        NodeDataOwner[size_t(*PredId)].Source->getParent() ==
            Succ->getParent() &&
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
      NodeAdjOwner[size_t(SuccVtxNode)].PredecessorIdx =
          PredId.value_or(NodeId::NoPredId);
      NodeDataOwner[size_t(SuccVtxNode)].Source = Curr;
      return;
    }

    // This node has more than one predecessor; add a neighbor then
    if (SuccVtx.predecessor().id() != PredId.value_or(NodeId::NoPredId) &&
        llvm::none_of(SuccVtx.neighbors(),
                      [Pred = PredId.value_or(NodeId::NoPredId)](NodeRef Nd) {
                        return Nd.predecessor().id() == Pred;
                      })) {

      auto NewNode = makeNode();
      NodeAdjOwner[size_t(SuccVtxNode)].Neighbors.push_back(NewNode);
      return;
    }
  }

  std::vector<NodeData> NodeDataOwner;
  std::vector<NodeAdj> NodeAdjOwner;
  std::unordered_map<std::pair<n_t, d_t>, NodeId, PathInfoHash, PathInfoEq>
      FlowFactVertexMap{};

  // ZeroValue
  d_t ZeroValue;
};

} // namespace psr

namespace llvm {
template <> struct DenseMapInfo<psr::detail::ExplodedSuperGraphNodeId> {
  using NodeId = psr::detail::ExplodedSuperGraphNodeId;

  static NodeId getEmptyKey() noexcept { return NodeId(-16); }
  static NodeId getTombstoneKey() noexcept { return NodeId(-32); }
  static auto getHashValue(NodeId Id) noexcept {
    return llvm::hash_value(std::underlying_type_t<NodeId>(Id));
  }
  static bool isEqual(NodeId L, NodeId R) noexcept { return L == R; }
};
} // namespace llvm

#endif // PHASAR_DATAFLOW_PATHSENSITIVITY_EXPLODEDSUPERGRAPH_H
