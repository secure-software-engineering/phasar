/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H
#define PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H

#include <memory_resource>

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/Pointer/DynamicPointsToSetPtr.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Utils/StableVector.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {
template <typename PointsToSetTy> class PointsToSetOwner {
public:
  PointsToSetOwner(std::pmr::polymorphic_allocator<PointsToSetTy> Alloc =
                       std::pmr::get_default_resource()) noexcept
      : Alloc(Alloc), AllPTS(Alloc.resource()) {}
  PointsToSetOwner(PointsToSetOwner &&) noexcept = default;

  PointsToSetOwner(const PointsToSetOwner &) = delete;
  PointsToSetOwner &operator=(const PointsToSetOwner &) = delete;
  PointsToSetOwner &operator=(PointsToSetOwner &&) = delete;

  ~PointsToSetOwner() {
    for (auto PTS : OwnedPTS) {
      std::destroy_at(PTS);
      Alloc.deallocate(PTS, 1);
    }
    OwnedPTS.clear();
  }

  DynamicPointsToSetPtr<PointsToSetTy> acquire() {
    auto Ptr = new (Alloc.allocate(1)) PointsToSetTy();
    OwnedPTS.insert(Ptr);
    return &AllPTS.emplace_back(Ptr);
  }
  void release(PointsToSetTy *PTS) noexcept {
    if (LLVM_UNLIKELY(!OwnedPTS.erase(PTS))) {
      llvm::report_fatal_error(
          "ERROR: release PointsToSet that was either already "
          "freed, or never allocated with this PointsToSetOwner!");
    }
    std::destroy_at(PTS);
    Alloc.deallocate(PTS, 1);
    /// NOTE: Do not delete from AllPTS!
  }

  void reserve(size_t Capacity) { OwnedPTS.reserve(Capacity); }

private:
  std::pmr::polymorphic_allocator<PointsToSetTy> Alloc;
  llvm::DenseSet<PointsToSetTy *> OwnedPTS;
  StableVector<PointsToSetTy *> AllPTS;
};

extern template class PointsToSetOwner<LLVMPointsToInfo::PointsToSetTy>;
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H
