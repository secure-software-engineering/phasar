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

#include "phasar/PhasarLLVM/Pointer/DynamicPointsToSetPtr.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/ErrorHandling.h"

#include <functional>
#include <memory>
#include <type_traits>

/// On some MAC systems, <memory_resource> is still not fully implemented, so do
/// a workaround here

#if !defined(__has_include) || __has_include(<memory_resource>)
#define HAS_MEMORY_RESOURCE 1
#include <memory_resource>
#else
#define HAS_MEMORY_RESOURCE 0
#include "llvm/Support/RecyclingAllocator.h"
#endif

namespace llvm {
class Value;
} // namespace llvm

namespace psr {
template <typename PointsToSetTy> class PointsToSetOwner {
public:
  using allocator_type =
#if HAS_MEMORY_RESOURCE
      std::pmr::polymorphic_allocator<PointsToSetTy>
#else
      llvm::RecyclingAllocator<llvm::BumpPtrAllocator, PointsToSetTy> *
#endif
      ;

  using memory_resource_type =
#if HAS_MEMORY_RESOURCE
      std::pmr::unsynchronized_pool_resource
#else
      llvm::RecyclingAllocator<llvm::BumpPtrAllocator, PointsToSetTy>
#endif
      ;

  PointsToSetOwner(allocator_type Alloc) noexcept : Alloc(Alloc) {
    if constexpr (std::is_pointer_v<allocator_type>) {
      assert(Alloc != nullptr);
    }
  }
  PointsToSetOwner(PointsToSetOwner &&) noexcept = default;

  PointsToSetOwner(const PointsToSetOwner &) = delete;
  PointsToSetOwner &operator=(const PointsToSetOwner &) = delete;
  PointsToSetOwner &operator=(PointsToSetOwner &&) = delete;

  ~PointsToSetOwner() {
    for (auto PTS : OwnedPTS) {
      std::destroy_at(PTS);
#if HAS_MEMORY_RESOURCE
      Alloc.deallocate(PTS, 1);
#else
      Alloc->Deallocate(PTS);
#endif
    }
    OwnedPTS.clear();
  }

  DynamicPointsToSetPtr<PointsToSetTy> acquire() {
    auto RawMem =
#if HAS_MEMORY_RESOURCE
        Alloc.allocate(1)
#else
        Alloc->Allocate()
#endif
        ;
    auto Ptr = new (RawMem) PointsToSetTy();
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
#if HAS_MEMORY_RESOURCE
    Alloc.deallocate(PTS, 1);
#else
    Alloc->Deallocate(PTS);
#endif
    /// NOTE: Do not delete from AllPTS!
  }

  void reserve(size_t Capacity) { OwnedPTS.reserve(Capacity); }

  [[nodiscard]] auto getAllPointsToSets() const noexcept {
    return llvm::make_range(OwnedPTS.begin(), OwnedPTS.end());
  }

private:
  allocator_type Alloc{};
  llvm::DenseSet<PointsToSetTy *> OwnedPTS;
  StableVector<PointsToSetTy *> AllPTS;
};

extern template class PointsToSetOwner<LLVMPointsToInfo::PointsToSetTy>;
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H
