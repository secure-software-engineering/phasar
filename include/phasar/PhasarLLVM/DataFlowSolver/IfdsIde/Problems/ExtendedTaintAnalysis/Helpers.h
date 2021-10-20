/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_HELPERS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_HELPERS_H_

#include <functional>
#include <map>
#include <memory>
#include <set>

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"

namespace llvm {
class Instruction;
class hash_code;
} // namespace llvm

namespace psr::XTaint {

/// Holds all leaks found during the analysis; Maybe use some better
/// datastructures...
using LeakMap_t = std::unordered_map<const llvm::Instruction *,
                                     llvm::SmallSet<const llvm::Value *, 1>>;

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType
getGenEdgeFunction(BasicBlockOrdering &BBO);

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType
getEdgeIdentity(const llvm::Instruction *Inst);
bool isEdgeIdentity(EdgeFunction<EdgeDomain> *EF);

llvm::hash_code
getHashCode(const EdgeFunction<EdgeDomain>::EdgeFunctionPtrType &EF);

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllTop();
EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllBot();
EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllSanitized();

/// Have an own function for creating a flow/edge-function instance to allow
/// fast migration to memory-management schemes other than std::shared_ptr
template <typename FlowFunctionTy, typename... Args>
inline std::shared_ptr<FlowFunctionTy> makeFF(Args &&...args) {
  return std::make_shared<FlowFunctionTy>(std::forward<Args>(args)...);
}
template <typename EdgeFunctionTy, typename... Args>
inline std::shared_ptr<EdgeFunctionTy> makeEF(Args &&...args) {
  return std::make_shared<EdgeFunctionTy>(std::forward<Args>(args)...);
}

} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_HELPERS_H_