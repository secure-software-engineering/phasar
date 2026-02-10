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
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/PointsToInfo.h"
#include "phasar/Pointer/PointsToIterator.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

#include <cstdint>

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

enum class SVFPointsToAnalysisType {
  DDA = int(AliasAnalysisType::SVFDDA),
  VFS = int(AliasAnalysisType::SVFVFS),
};

using SVFBasedPointsToInfo = PointsToInfo<SVFPointsToInfoTraits>;
using SVFBasedPointsToInfoRef = PointsToInfoRef<SVFPointsToInfoTraits>;
using SVFBasedPointsToIterator =
    PointsToIterator<const llvm::Value *, uint32_t, const llvm::Instruction *>;
using SVFBasedPointsToIteratorRef =
    PointsToIteratorRef<const llvm::Value *, uint32_t,
                        const llvm::Instruction *>;

/// Use SVF to perform a VersionedFlowSensitive pointer analysis and return the
/// results compatible to psr::PointsToInfo and psr::PointsToInfoRef
[[nodiscard]] SVFBasedPointsToInfo
createSVFVFSPointsToInfo(LLVMProjectIRDB &IRDB);

/// Use SVF to perform a ContextDDA pointer analysis and return the
/// results compatible to psr::PointsToInfo and psr::PointsToInfoRef
[[nodiscard]] SVFBasedPointsToInfo
createSVFDDAPointsToInfo(LLVMProjectIRDB &IRDB);

/// Use SVF to perform the specified pointer analysis and return the results
/// compatible to psr::PointsToInfo and psr::PointsToInfoRef
[[nodiscard]] SVFBasedPointsToInfo
createSVFPointsToInfo(LLVMProjectIRDB &IRDB, SVFPointsToAnalysisType PTATy);

/// Use SVF to perform the specified pointer analysis and return the results
/// compatible to psr::LLVMPointsToIterator, converting the points-to sets to
/// LLVM allocation-sites
[[nodiscard]] LLVMPointsToIterator
createLLVMSVFPointsToIterator(LLVMProjectIRDB &IRDB,
                              SVFPointsToAnalysisType PTATy);

/// Use SVF to perform a ContextDDA pointer analysis and return the results
/// compatible to psr::LLVMAliasInfo and psr::LLVMAliasInfoRef
///
/// \note Only support DDA for now, as VFS seems to not support getRevPts().
[[nodiscard]] LLVMAliasInfo createLLVMSVFDDAAliasInfo(LLVMProjectIRDB &IRDB);

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVF_SVFPOINTSTOSET_H
