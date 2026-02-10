/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_TYPEASSIGNMENTGRAPH_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_TYPEASSIGNMENTGRAPH_H

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <variant>

namespace psr {
class Resolver;
class LLVMProjectIRDB;
class LLVMVFTableProvider;
} // namespace psr

namespace llvm {
class DIType;
class Value;
class Function;
} // namespace llvm

namespace psr::vta {

enum class TAGNodeId : uint32_t {};

struct Variable {
  const llvm::Value *Val;
};

struct Field {
  const llvm::DIType *Base;
  size_t ByteOffset;
};

struct Return {
  const llvm::Function *Fun;
};

struct TAGNode {
  std::variant<Variable, Field, Return> Label;
};

constexpr bool operator==(Variable L, Variable R) noexcept {
  return L.Val == R.Val;
}
constexpr bool operator==(Field L, Field R) noexcept {
  return L.Base == R.Base && L.ByteOffset == R.ByteOffset;
}
constexpr bool operator==(Return L, Return R) noexcept {
  return L.Fun == R.Fun;
}
constexpr bool operator==(TAGNode L, TAGNode R) noexcept {
  return L.Label == R.Label;
}
}; // namespace psr::vta

namespace llvm {
template <> struct DenseMapInfo<psr::vta::TAGNode> {
  using TAGNode = psr::vta::TAGNode;
  using Variable = psr::vta::Variable;
  using Field = psr::vta::Field;
  using Return = psr::vta::Return;

  inline static TAGNode getEmptyKey() noexcept {
    return {Variable{llvm::DenseMapInfo<const llvm::Value *>::getEmptyKey()}};
  }
  inline static TAGNode getTombstoneKey() noexcept {
    return {
        Variable{llvm::DenseMapInfo<const llvm::Value *>::getTombstoneKey()}};
  }
  inline static bool isEqual(TAGNode L, TAGNode R) noexcept { return L == R; }
  inline static auto getHashValue(TAGNode TN) noexcept {
    if (const auto *Var = std::get_if<Variable>(&TN.Label)) {
      return llvm::hash_combine(0, Var->Val);
    }
    if (const auto *Fld = std::get_if<Field>(&TN.Label)) {
      return llvm::hash_combine(1, Fld->Base, Fld->ByteOffset);
    }
    if (const auto *Ret = std::get_if<Return>(&TN.Label)) {
      return llvm::hash_combine(2, Ret->Fun);
    }
    llvm_unreachable("All TAGNode variants should be handled already");
  }
};

template <> struct DenseMapInfo<psr::vta::TAGNodeId> {
  using GraphNodeId = psr::vta::TAGNodeId;
  inline static GraphNodeId getEmptyKey() noexcept { return GraphNodeId(-1); }
  inline static GraphNodeId getTombstoneKey() noexcept {
    return GraphNodeId(-2);
  }
  inline static bool isEqual(GraphNodeId L, GraphNodeId R) noexcept {
    return L == R;
  }
  inline static auto getHashValue(GraphNodeId TN) noexcept {
    return llvm::hash_value(uint32_t(TN));
  }
};

} // namespace llvm

namespace psr::vta {

struct TypeAssignmentGraph {
  using GraphNodeId = TAGNodeId;
  using TypeInfoTy =
      llvm::PointerUnion<const llvm::Function *, const llvm::DIType *>;
  Compressor<TAGNode, GraphNodeId> Nodes;

  TypedVector<TAGNodeId, llvm::SmallDenseSet<TAGNodeId>> Adj;
  llvm::SmallDenseMap<TAGNodeId, llvm::SmallDenseSet<TypeInfoTy>>
      TypeEntryPoints;

  [[nodiscard]] inline std::optional<TAGNodeId> get(TAGNode TN) const noexcept {
    return Nodes.getOrNull(TN);
  }

  [[nodiscard]] inline TAGNode operator[](TAGNodeId Id) const noexcept {
    return Nodes[Id];
  }

  inline void addEdge(TAGNodeId From, TAGNodeId To) {
    assert(size_t(From) < Adj.size());
    assert(size_t(To) < Adj.size());

    if (From == To) {
      return;
    }

    Adj[From].insert(To);
  }

  void print(llvm::raw_ostream &OS);
};

using ReachableFunsHandlerTy = llvm::function_ref<void(const llvm::Function *)>;
using ReachableFunsTy =
    llvm::function_ref<void(const LLVMProjectIRDB &, ReachableFunsHandlerTy)>;

[[nodiscard]] TypeAssignmentGraph
computeTypeAssignmentGraph(const LLVMProjectIRDB &IRDB,
                           const psr::LLVMVFTableProvider &VTP,
                           LLVMAliasIteratorRef AS, Resolver &BaseRes,
                           ReachableFunsTy ReachableFunctions);

void printNode(llvm::raw_ostream &OS, TAGNode TN);
}; // namespace psr::vta

namespace psr {
template <> struct GraphTraits<vta::TypeAssignmentGraph> {
  using graph_type = vta::TypeAssignmentGraph;
  using value_type = vta::TAGNode;
  using vertex_t = vta::TAGNodeId;
  using edge_t = vertex_t;

  static constexpr vertex_t Invalid = vertex_t(UINT32_MAX);

  [[nodiscard]] static const auto &outEdges(const graph_type &G,
                                            vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    return G.Adj[Vtx];
  }
  [[nodiscard]] static size_t outDegree(const graph_type &G,
                                        vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    return G.Adj[Vtx].size();
  }

  [[nodiscard]] static const auto &nodes(const graph_type &G) noexcept {
    return G.Nodes;
  }

  [[nodiscard]] static auto roots(const graph_type &G) noexcept {
    return llvm::make_first_range(G.TypeEntryPoints);
  }

  [[nodiscard]] static auto vertices(const graph_type &G) noexcept {
    return iota<vertex_t>(G.Adj.size());
  }

  [[nodiscard]] static value_type node(const graph_type &G,
                                       vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }

  [[nodiscard]] static size_t size(const graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Adj.size();
  }

  [[nodiscard]] static size_t
  roots_size(const graph_type &G) noexcept { // NOLINT
    return G.TypeEntryPoints.size();
  }

  [[nodiscard]] static vertex_t target(edge_t Edge) noexcept { return Edge; }

  [[nodiscard]] static vertex_t withEdgeTarget(edge_t /*Edge*/,
                                               vertex_t NewTgt) noexcept {
    return NewTgt;
  }
};
} // namespace psr

#endif
