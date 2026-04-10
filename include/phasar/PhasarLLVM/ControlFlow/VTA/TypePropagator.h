/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_TYPEPROPAGATOR_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_TYPEPROPAGATOR_H

#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {
template <typename GraphNodeId> struct SCCHolder;
template <typename GraphNodeId> struct SCCDependencyGraph;
template <typename GraphNodeId> struct SCCOrder;
} // namespace psr

namespace psr::vta {
struct TypeAssignmentGraph;
enum class TAGNodeId : uint32_t;

/// \brief A concrete type-assignment that assigns a set of possible types to
/// each SCC of the TypeAssignmentGraph
struct TypeAssignment {
  TypedVector<SCCId<TAGNodeId>,
              llvm::SmallDenseSet<llvm::PointerUnion<const llvm::Function *,
                                                     const llvm::DIType *>>>
      TypesPerSCC;

  void print(llvm::raw_ostream &OS, const TypeAssignmentGraph &TAG,
             const SCCHolder<TypeAssignmentGraph::GraphNodeId> &SCCs);
};

/// Computes a TypeAssignment, based on a given TypeAssignmentGraph
[[nodiscard]] TypeAssignment
propagateTypes(const TypeAssignmentGraph &TAG, const SCCHolder<TAGNodeId> &SCCs,
               const SCCDependencyGraph<TAGNodeId> &Deps,
               const SCCOrder<TAGNodeId> &Order);

} // namespace psr::vta
#endif
