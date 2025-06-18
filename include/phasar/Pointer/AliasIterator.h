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
#include "phasar/Pointer/AliasInfoBase.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Utils/ByRef.h"
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

  [[nodiscard]] AliasResult alias(v_t Ptr, v_t Alias, n_t At) {
    assert(AA != nullptr);

    if (Ptr == Alias) {
      return AliasResult::MustAlias;
    }

    if (AA->isInReachableAllocationSites(Ptr, Alias, true, At)) {
      return AliasResult::MayAlias;
    }

    return AliasResult::NoAlias;
  }

  UnderlyingAA *AA{};
};

template <typename UnderlyingAA>
ReachableAllocationSitesIterator(UnderlyingAA *)
    -> ReachableAllocationSitesIterator<UnderlyingAA>;

template <typename V, typename N> class AliasIteratorRef {
  template <typename ConcreteAA>
  static constexpr bool CanSSO = std::is_trivially_copyable_v<ConcreteAA> &&
                                 sizeof(ConcreteAA) <= sizeof(void *);

  template <typename ConcreteAA>
  constexpr static ConcreteAA *fromOpaquePtr(void *&EF) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      return static_cast<ConcreteAA *>(static_cast<void *>(&EF));
    } else {
      return static_cast<ConcreteAA *>(EF);
    }
  }

  template <typename ConcreteAA>
  constexpr void *getOpaquePtr(ConcreteAA &AA) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      void *Ret{};
      ::new (&Ret) ConcreteAA(AA);
      return Ret;
    } else {
      return &AA;
    }
  }

public:
  using n_t = N;
  using v_t = V;

  template <typename ConcreteAA,
            typename = std::enable_if_t<
                !std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
                std::is_same_v<v_t, typename ConcreteAA::v_t> &&
                std::is_same_v<n_t, typename ConcreteAA::n_t>>>
  constexpr AliasIteratorRef(ConcreteAA *AA) noexcept
      : AA(getOpaquePtr(psr::assertNotNull(AA))), VT(&VtableFor<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }

  constexpr AliasIteratorRef(AliasInfoRef<V, N> AS) noexcept
      : AA(&psr::assertNotNull(AS.AA)), VT(AS.VT->AliasesOf, AS.VT.Alias) {}

  constexpr AliasIteratorRef(const AliasIteratorRef &) noexcept = default;
  constexpr AliasIteratorRef &
  operator=(const AliasIteratorRef &) noexcept = default;
  ~AliasIteratorRef() = default;

  void aliasesOf(ByConstRef<v_t> Of, ByConstRef<n_t> At,
                 llvm::function_ref<void(v_t)> WithAlias) {
    assert(VT != nullptr);
    VT->AliasesOf(AA, Of, At, WithAlias);
  }

  template <typename SetT = std::set<v_t>>
  [[nodiscard]] SetT asSet(ByConstRef<v_t> Of, ByConstRef<n_t> At) {
    SetT Set;
    aliasesOf(Of, At, [&Set](v_t Alias) { Set.insert(std::move(Alias)); });
    return Set;
  }

  [[nodiscard]] AliasResult alias(ByConstRef<v_t> Ptr, ByConstRef<v_t> Alias,
                                  ByConstRef<n_t> At) {
    assert(VT != nullptr);
    return VT->Alias(AA, Ptr, Alias, At);
  }

private:
  struct VTable {
    void (*AliasesOf)(void *, ByConstRef<v_t>, ByConstRef<n_t>,
                      llvm::function_ref<void(v_t)>);
    AliasResult (*Alias)(void *, ByConstRef<v_t>, ByConstRef<v_t>,
                         ByConstRef<n_t>);
  };

  template <typename ConcreteAA>
  static void aliasesOfThunk(void *AA, ByConstRef<v_t> Of, ByConstRef<n_t> At,
                             llvm::function_ref<void(v_t)> WithAlias) {
    const auto *CAA = fromOpaquePtr<ConcreteAA>(AA);
    if constexpr (IsAliasIterator<ConcreteAA>) {
      return CAA->aliasesof(Of, At, WithAlias);
    } else {
      auto AliasSetPtr = CAA->getAliasSet(Of, At);
      for (auto &&Alias : *AliasSetPtr) {
        WithAlias(PSR_FWD(Alias));
      }
    }
  }

  template <typename ConcreteAA>
  static AliasResult aliasThunk(void *AA, ByConstRef<v_t> Ptr,
                                ByConstRef<v_t> Alias, ByConstRef<n_t> At) {
    if (Ptr == Alias) {
      return AliasResult::MustAlias;
    }

    const auto *CAA = fromOpaquePtr<ConcreteAA>(AA);
    if constexpr (detail::HasAlias<ConcreteAA>::value) {
      return CAA->alias(Ptr, Alias, At);
    } else if constexpr (detail::HasGetAliasSet<ConcreteAA>::value) {
      auto AliasSetPtr = CAA->getAliasSet(Ptr, At);
      return AliasSetPtr->count(Alias);
    } else {
      AliasResult Ret = AliasResult::NoAlias;

      CAA->aliasesof(Ptr, At, [&Ret, Alias](v_t A) {
        if (A == Alias) {
          Ret = AliasResult::MayAlias;
        }
      });

      return Ret;
    }
  }

  template <typename ConcreteAA>
  constexpr static VTable VtableFor = {
      &aliasesOfThunk<ConcreteAA>,
      &aliasThunk<ConcreteAA>,
  };

  void *AA{};
  const VTable *VT{};
}; // namespace psr

} // namespace psr

#endif // PHASAR_POINTER_ALIASITERATOR_H
