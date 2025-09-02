/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/VTA/TypePropagator.h"

#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/IR/DebugInfoMetadata.h"

using namespace psr;
using namespace psr::vta;

static void initialize(TypeAssignment &TA, const TypeAssignmentGraph &TAG,
                       const SCCHolder<TAGNodeId> &SCCs) {
  for (const auto &[Node, Types] : TAG.TypeEntryPoints) {
    auto SCC = SCCs.SCCOfNode[Node];
    TA.TypesPerSCC[SCC].insert(Types.begin(), Types.end());
  }
}

static void propagate(TypeAssignment &TA,
                      const SCCDependencyGraph<TAGNodeId> &Deps,
                      SCCId<TAGNodeId> CurrSCC) {
  const auto &Types = TA.TypesPerSCC[CurrSCC];
  if (Types.empty()) {
    return;
  }

  for (auto Succ : Deps.ChildrenOfSCC[CurrSCC]) {
    TA.TypesPerSCC[Succ].insert(Types.begin(), Types.end());
  }
}

TypeAssignment vta::propagateTypes(const TypeAssignmentGraph &TAG,
                                   const SCCHolder<TAGNodeId> &SCCs,
                                   const SCCDependencyGraph<TAGNodeId> &Deps,
                                   const SCCOrder<TAGNodeId> &Order) {
  TypeAssignment Ret;
  Ret.TypesPerSCC.resize(SCCs.size());

  initialize(Ret, TAG, SCCs);
  for (auto SCC : Order.SCCIds) {
    propagate(Ret, Deps, SCC);
  }

  return Ret;
}

void TypeAssignment::print(llvm::raw_ostream &OS,
                           const TypeAssignmentGraph &TAG,
                           const SCCHolder<TAGNodeId> &SCCs) {
  OS << "digraph TypeAssignment {\n";
  psr::scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  Compressor<TypeAssignmentGraph::TypeInfoTy> Types;
  auto GetOrAddType = [&](TypeAssignmentGraph::TypeInfoTy Ty) {
    auto [Id, Inserted] = Types.insert(Ty);
    if (Inserted) {
      OS << (size_t(Id) + SCCs.size()) << "[label=\"";
      if (const auto *Fun = Ty.dyn_cast<const llvm::Function *>()) {
        OS << "fun-" << Fun->getName();
      } else if (const auto *DITy = Ty.dyn_cast<const llvm::DIType *>()) {
        OS << "type-";
        OS.write_escaped(llvmTypeToString(DITy, true));
      }
      OS << "\"];\n";
    }
    return Id + SCCs.size();
  };

  for (const auto &[Ctr, NodesInSCC] : SCCs.NodesInSCC.enumerate()) {
    OS << "  " << uint32_t(Ctr) << "[label=\"";
    for (auto TNId : SCCs.NodesInSCC[Ctr]) {
      auto TN = TAG.Nodes[TNId];
      printNode(OS, TN);
      OS << "\\n";
    }
    OS << "\"];\n";

    for (auto Ty : TypesPerSCC[Ctr]) {
      auto TyId = GetOrAddType(Ty);
      OS << uint32_t(Ctr) << "->" << TyId << ";\n";
    }
  }
}
