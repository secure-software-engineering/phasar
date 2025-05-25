/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/
#ifndef PHASAR_PHASARLLVM_POINTER_SVF_SVFPOINTSTOSET_H
#define PHASAR_PHASARLLVM_POINTER_SVF_SVFPOINTSTOSET_H

#include "phasar/Config/phasar-config.h"
#include "phasar/Pointer/PointsToInfo.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

#ifndef PHASAR_USE_SVF
#error                                                                         \
    "Don't include SVFPointsToSet.h when PhASAR is not configured to include SVF. Set the cmake variable PHASAR_USE_SVF and retry."
#endif

namespace psr {
class LLVMProjectIRDB;
class SVFPointsToInfo;

struct SVFPointsToInfoTraits {
  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using o_t = uint32_t;

  // TODO: Use a more efficient representation; maybe even one that does not
  // require an expensive transformation from SVF::PointsTo
  using PointsToSetTy = llvm::SmallDenseSet<o_t>;

  // No special pointer type
  using PointsToSetPtrTy = PointsToSetTy;
};

using SVFBasedPointsToInfo = PointsToInfo<SVFPointsToInfoTraits>;
using SVFBasedPointsToInfoRef = PointsToInfoRef<SVFPointsToInfoTraits>;

/// Use SVF to perform a VersionedFlowSensitive pointer analysis and return the
/// results compatible to psr::PointsToInfo and psr::PointsToInfoRef
[[nodiscard]] SVFBasedPointsToInfo
createSVFVFSPointsToInfo(LLVMProjectIRDB &IRDB);

/// Use SVF to perform a ContextDDA pointer analysis and return the
/// results compatible to psr::PointsToInfo and psr::PointsToInfoRef
[[nodiscard]] SVFBasedPointsToInfo
createSVFDDAPointsToInfo(LLVMProjectIRDB &IRDB);

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVF_SVFPOINTSTOSET_H
