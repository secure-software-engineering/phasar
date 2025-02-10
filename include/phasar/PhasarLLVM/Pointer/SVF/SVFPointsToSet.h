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
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/PointsToInfoBase.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

#ifndef PHASAR_USE_SVF
#error                                                                         \
    "Don't include SVFPointsToSet.h when PhASAR is not configured to include SVF. Set the cmake variable PHASAR_USE_SVF and retry."
#endif

namespace psr {
class LLVMProjectIRDB;
class SVFPointsToSet;

template <> struct PointsToTraits<SVFPointsToSet> {
  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using o_t = uint32_t;

  // TODO: Use a more efficient representation; maybe even one that does not
  // require an expensive transformation from SVF::PointsTo
  using PointsToSetTy = llvm::SmallDenseSet<o_t>;

  // Not special pointer type
  using PointsToSetPtrTy = PointsToSetTy;
};

class SVFPointsToSet : public PointsToInfoBase<SVFPointsToSet> {
  friend PointsToInfoBase<SVFPointsToSet>;

public:
  explicit SVFPointsToSet(const LLVMProjectIRDB *IRDB,
                          AliasAnalysisType PAType = AliasAnalysisType::SVFVFS);

private:
  [[nodiscard]] o_t
  asAbstractObjectImpl(ByConstRef<v_t> Pointer) const noexcept;

  [[nodiscard]] std::optional<v_t> asPointerOrNullImpl(o_t Obj) const noexcept;

  bool mayPointsToImpl(o_t Pointer, o_t Obj, n_t AtInstruction) const;
  bool mayPointsToImpl(v_t Pointer, o_t Obj, n_t AtInstruction) const;

  PointsToSetPtrTy getPointsToSetImpl(o_t Pointer, n_t AtInstruction) const;
  PointsToSetPtrTy getPointsToSetImpl(v_t Pointer, n_t AtInstruction) const;

  llvm::SmallVector<v_t>
  getInterestingPointersAtImpl(ByConstRef<n_t> AtInstruction) const;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVF_SVFPOINTSTOSET_H
