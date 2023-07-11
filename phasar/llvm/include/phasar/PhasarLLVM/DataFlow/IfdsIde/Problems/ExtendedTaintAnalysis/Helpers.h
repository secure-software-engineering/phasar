/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_HELPERS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_HELPERS_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"

#include <functional>
#include <map>
#include <memory>
#include <set>

namespace llvm {
class Instruction;
class hash_code;
} // namespace llvm

namespace psr::XTaint {

/// Holds all leaks found during the analysis; Maybe use some better
/// datastructures...
using LeakMap_t = std::unordered_map<const llvm::Instruction *,
                                     llvm::SmallSet<const llvm::Value *, 1>>;

/// Have an own function for creating a flow/edge-function instance to allow
/// fast migration to memory-management schemes other than std::shared_ptr
template <typename FlowFunctionTy, typename... Args>
inline std::shared_ptr<FlowFunctionTy> makeFF(Args &&...Arguments) {
  return std::make_shared<FlowFunctionTy>(std::forward<Args>(Arguments)...);
}

} // namespace psr::XTaint

#endif
