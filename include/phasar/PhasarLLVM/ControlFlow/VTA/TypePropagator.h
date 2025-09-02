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
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {
template <typename GraphNodeId> struct SCCId;
template <typename GraphNodeId> struct SCCHolder;
template <typename GraphNodeId> struct SCCDependencyGraph;
template <typename GraphNodeId> struct SCCOrder;
} // namespace psr

namespace psr::vta {
struct TypeAssignmentGraph;
enum class TAGNodeId : uint32_t;

struct TypeAssignment {
  TypedVector<SCCId<TAGNodeId>, llvm::SmallDenseSet<const llvm::Value *>>
      TypesPerSCC;

  void print(llvm::raw_ostream &OS, const TypeAssignmentGraph &TAG,
             const SCCHolder<TypeAssignmentGraph::GraphNodeId> &SCCs);
};

[[nodiscard]] TypeAssignment
propagateTypes(const TypeAssignmentGraph &TAG, const SCCHolder<TAGNodeId> &SCCs,
               const SCCDependencyGraph<TAGNodeId> &Deps,
               const SCCOrder<TAGNodeId> &Order);

} // namespace psr::vta
#endif
