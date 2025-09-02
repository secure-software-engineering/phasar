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
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Compressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <variant>

namespace psr::vta {

enum class TAGNodeId : uint32_t {};

struct Variable {
  const llvm::Value *Val;
};

struct Field {
  const llvm::Type *Base;
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
  Compressor<TAGNode, GraphNodeId> Nodes;

  llvm::SmallVector<llvm::SmallDenseSet<GraphNodeId>, 0> Adj;
  llvm::SmallDenseMap<GraphNodeId, llvm::SmallDenseSet<const llvm::Value *>>
      TypeEntryPoints;

  [[nodiscard]] inline std::optional<GraphNodeId>
  get(TAGNode TN) const noexcept {
    return Nodes.getOrNull(TN);
  }

  [[nodiscard]] inline TAGNode operator[](GraphNodeId Id) const noexcept {
    return Nodes[Id];
  }

  inline void addEdge(GraphNodeId From, GraphNodeId To) {
    assert(size_t(From) < Adj.size());
    assert(size_t(To) < Adj.size());

    if (From == To) {
      return;
    }

    Adj[size_t(From)].insert(To);
  }

  void print(llvm::raw_ostream &OS);
};

using AliasHandlerTy = llvm::function_ref<void(const llvm::Value *)>;
using AliasInfoTy = llvm::function_ref<void(
    const llvm::Value *, const llvm::Instruction *, AliasHandlerTy)>;

// TODO: Use AliasIterator here, once available
[[nodiscard]] TypeAssignmentGraph computeTypeAssignmentGraph(
    const llvm::Module &Mod,
    const psr::CallGraph<const llvm::Instruction *, const llvm::Function *>
        &BaseCG,
    AliasInfoTy AS, const psr::LLVMVFTableProvider &VTP);

void printNode(llvm::raw_ostream &OS, TAGNode TN);
}; // namespace psr::vta

#endif
