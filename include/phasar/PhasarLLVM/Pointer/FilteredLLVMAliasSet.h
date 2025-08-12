/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASSET_H
#define PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASSET_H

#include "phasar/PhasarLLVM/Pointer/CachedLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"

namespace llvm {
class Value;
class Instruction;
} // namespace llvm

namespace psr {

class FilteredLLVMAliasSet;

template <>
struct AliasInfoTraits<FilteredLLVMAliasSet>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

class FilteredLLVMAliasSet : private FilteredLLVMAliasIterator,
                             public CachedLLVMAliasIterator {
public:
  using typename CachedLLVMAliasIterator::alias_traits_t;
  using typename CachedLLVMAliasIterator::AliasSetPtrTy;
  using typename CachedLLVMAliasIterator::AliasSetTy;
  using typename CachedLLVMAliasIterator::AllocationSiteSetPtrTy;
  using typename CachedLLVMAliasIterator::n_t;
  using typename CachedLLVMAliasIterator::v_t;

  FilteredLLVMAliasSet(LLVMAliasIteratorRef Underlying) noexcept
      : FilteredLLVMAliasIterator(Underlying),
        CachedLLVMAliasIterator(
            static_cast<FilteredLLVMAliasIterator *>(this)) {}
};

static_assert(std::is_convertible_v<FilteredLLVMAliasSet *, LLVMAliasInfoRef>);
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASSET_H
