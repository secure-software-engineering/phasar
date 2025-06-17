/******************************************************************************
 * Copyright (c) 2025 Fabian Schíebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_ALIASITERATOR_H
#define PHASAR_POINTER_ALIASITERATOR_H

#include "phasar/Pointer/AliasInfo.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <cassert>
#include <type_traits>

namespace psr {

template <typename UnderlyingAA> struct ReachableAllocationSitesIterator {
  using n_t = typename UnderlyingAA::n_t;
  using v_t = typename UnderlyingAA::v_t;

  void aliasesOf(v_t Of, n_t At, llvm::function_ref<void(v_t)> WithAlias) {
    assert(AA != nullptr);

    auto AliasSetPtr = AA->getReachableAllocationSites(Of, true, At);
    if (!AliasSetPtr || AliasSetPtr->empty()) {
      // The alias-relation should be reflexive
      WithAlias(Of);
      return;
    }

    for (auto &&Alias : *AliasSetPtr) {
      WithAlias(PSR_FWD(Alias));
    }
  }

  UnderlyingAA *AA{};
};

template <typename UnderlyingAA>
ReachableAllocationSitesIterator(UnderlyingAA *)
    -> ReachableAllocationSitesIterator<UnderlyingAA>;

template <typename V, typename N> class AliasIteratorRef {
public:
  using n_t = N;
  using v_t = V;

  template <typename ConcreteAA,
            typename = std::enable_if_t<
                !std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
                std::is_same_v<v_t, typename ConcreteAA::v_t> &&
                std::is_same_v<n_t, typename ConcreteAA::n_t>>>
  constexpr AliasIteratorRef(ConcreteAA *AA) noexcept
      : AA(&psr::assertNotNull(AA)), AliasesOf(&aliasesOfThunk<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }

  template <typename UnderlyingAA>
  constexpr AliasIteratorRef(
      ReachableAllocationSitesIterator<UnderlyingAA> RAS) noexcept
      : AA(&psr::assertNotNull(RAS.AA)),
        AliasesOf(&allocSitesOfThunk<UnderlyingAA>) {}

  constexpr AliasIteratorRef(AliasInfoRef<V, N> AS) noexcept
      : AA(&psr::assertNotNull(AS.AA)), AliasesOf(AS.VT->AliasesOf) {}

  constexpr AliasIteratorRef(const AliasIteratorRef &) noexcept = default;
  constexpr AliasIteratorRef &
  operator=(const AliasIteratorRef &) noexcept = default;
  ~AliasIteratorRef() = default;

  void aliasesOf(v_t Of, n_t At, llvm::function_ref<void(v_t)> WithAlias) {
    assert(AliasesOf);
    AliasesOf(AA, std::move(Of), std::move(At), WithAlias);
  }

  template <typename SetT = std::set<v_t>>
  [[nodiscard]] SetT asSet(v_t Of, n_t At) {
    SetT Set;
    aliasesOf(Of, At, [&Set](v_t Alias) { Set.insert(std::move(Alias)); });
    return Set;
  }

private:
  template <typename ConcreteAA>
  static void aliasesOfThunk(void *AA, v_t Of, n_t At,
                             llvm::function_ref<void(v_t)> WithAlias) {
    if constexpr (IsAliasIterator<ConcreteAA>) {
      return static_cast<ConcreteAA *>(AA)->aliasesof(Of, At, WithAlias);
    } else {
      auto AliasSetPtr = static_cast<ConcreteAA *>(AA)->getAliasSet(Of, At);
      for (auto &&Alias : *AliasSetPtr) {
        WithAlias(PSR_FWD(Alias));
      }
    }
  }

  template <typename UnderlyingAA>
  static void allocSitesOfThunk(void *AA, v_t Of, n_t At,
                                llvm::function_ref<void(v_t)> WithAlias) {
    ReachableAllocationSitesIterator<UnderlyingAA>{
        static_cast<UnderlyingAA *>(AA)}
        .aliasesOf(Of, At, WithAlias);
  }

  void *AA{};
  void (*AliasesOf)(void *, v_t, n_t, llvm::function_ref<void(v_t)>){};
};

} // namespace psr

#endif // PHASAR_POINTER_ALIASITERATOR_H
