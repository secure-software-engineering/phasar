/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_ALIASSETOWNER_H
#define PHASAR_POINTER_ALIASSETOWNER_H

#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Utils/BoxedPointer.h"
#include "phasar/Utils/MemoryResource.h"
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

#if !HAS_MEMORY_RESOURCE
#include "llvm/Support/RecyclingAllocator.h"
#endif
namespace llvm {
class Value;
class Instruction;
} // namespace llvm

namespace psr {
template <typename AliasSetTy> class AliasSetOwner {
public:
  using allocator_type =
#if HAS_MEMORY_RESOURCE
      std::pmr::polymorphic_allocator<AliasSetTy>
#else
      llvm::RecyclingAllocator<llvm::BumpPtrAllocator, AliasSetTy> *
#endif
      ;

  using memory_resource_type =
#if HAS_MEMORY_RESOURCE
      std::pmr::unsynchronized_pool_resource
#else
      llvm::RecyclingAllocator<llvm::BumpPtrAllocator, AliasSetTy>
#endif
      ;

  AliasSetOwner(allocator_type Alloc) noexcept : Alloc(Alloc) {
    if constexpr (std::is_pointer_v<allocator_type>) {
      assert(Alloc != nullptr);
    }
  }
  AliasSetOwner(AliasSetOwner &&) noexcept = default;

  AliasSetOwner(const AliasSetOwner &) = delete;
  AliasSetOwner &operator=(const AliasSetOwner &) = delete;
  AliasSetOwner &operator=(AliasSetOwner &&) = delete;

  ~AliasSetOwner() {
    for (auto *PTS : OwnedPTS) {
      std::destroy_at(PTS);
#if HAS_MEMORY_RESOURCE
      Alloc.deallocate(PTS, 1);
#else
      Alloc->Deallocate(PTS);
#endif
    }
    OwnedPTS.clear();
  }

  BoxedPtr<AliasSetTy> acquire() {
    auto RawMem =
#if HAS_MEMORY_RESOURCE
        Alloc.allocate(1)
#else
        Alloc->Allocate()
#endif
        ;
    auto Ptr = new (RawMem) AliasSetTy();
    OwnedPTS.insert(Ptr);
    return &AllPTS.emplace_back(Ptr);
  }

  void release(AliasSetTy *PTS) noexcept {
    if (LLVM_UNLIKELY(!OwnedPTS.erase(PTS))) {
      llvm::report_fatal_error(
          "ERROR: release AliasSet that was either already "
          "freed, or never allocated with this AliasSetOwner!");
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

  [[nodiscard]] auto getAllAliasSets() const noexcept {
    return llvm::make_range(OwnedPTS.begin(), OwnedPTS.end());
  }

private:
  allocator_type Alloc{};
  llvm::DenseSet<AliasSetTy *> OwnedPTS;
  StableVector<AliasSetTy *> AllPTS;
};

extern template class AliasSetOwner<DefaultAATraits<
    const llvm::Value *, const llvm::Instruction *>::AliasSetTy>;
} // namespace psr

#endif // PHASAR_POINTER_ALIASSETOWNER_H
