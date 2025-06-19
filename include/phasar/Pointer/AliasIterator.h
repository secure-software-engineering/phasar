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

#include "phasar/Pointer/AliasResult.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/TypeErasureUtils.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <cassert>
#include <type_traits>

namespace psr {

namespace detail {

template <typename T, typename = void>
struct IsAliasIterator : std::false_type {};

template <typename T>
struct IsAliasIterator<
    T, std::void_t<decltype(std::declval<T>().aliasesOf(
           std::declval<typename T::v_t>(), std::declval<typename T::n_t>(),
           std::declval<llvm::function_ref<void(typename T::v_t)>>()))>>
    : std::true_type {};

template <typename T, typename = AliasResult>
struct HasAlias : std::false_type {};
template <typename T>
struct HasAlias<T, decltype(std::declval<T>().alias(
                       std::declval<typename T::v_t>(),
                       std::declval<typename T::v_t>(),
                       std::declval<typename T::n_t>()))> : std::true_type {};

template <typename T, typename = void>
struct HasGetAliasSet : std::false_type {};
template <typename T>
struct HasGetAliasSet<
    T, std::void_t<decltype(std::declval<T>().getAliasSet(
           std::declval<typename T::v_t>(), std::declval<typename T::n_t>()))>>
    : std::true_type {};

} // namespace detail
template <typename T>
PSR_CONCEPT IsAliasIterator = detail::IsAliasIterator<T>::value;

/// \brief A type-erased reference to any object implementing the
/// IsAliasIterator interface. Use this, if your alias-aware analysis just needs
/// a minimal interface to work with aliases and does not require the
/// versatility of AliasInfoRef.
///
/// This is a *non-owning* reference similar to std::string_view and
/// llvm::ArrayRef. Pass values of this type by value.
///
/// Example:
/// \code
/// LLVMAliasSet ASet(...);
/// LLVMAliasIteratorRef AA = &ASet;
/// \endcode
template <typename V, typename N>
class [[gsl::Pointer]] AliasIteratorRef : private TypeErasureUtils {
public:
  using n_t = N;
  using v_t = V;

  struct VTable {
    void (*AliasesOf)(void *, ByConstRef<V>, ByConstRef<N>,
                      llvm::function_ref<void(V)>);
    AliasResult (*Alias)(void *, ByConstRef<V>, ByConstRef<V>, ByConstRef<N>);
  };

  template <typename ConcreteAA,
            typename = std::enable_if_t<
                !std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
                std::is_same_v<v_t, typename ConcreteAA::v_t> &&
                std::is_same_v<n_t, typename ConcreteAA::n_t>>>
  constexpr AliasIteratorRef(ConcreteAA *AA) noexcept
      : AA(getOpaquePtr(psr::assertNotNull(AA))), VT(&VtableFor<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }
  template <
      typename ConcreteAA,
      typename = std::enable_if_t<
          !std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
          std::is_same_v<v_t, typename ConcreteAA::v_t> &&
          std::is_same_v<n_t, typename ConcreteAA::n_t> && CanSSO<ConcreteAA>>>
  constexpr AliasIteratorRef(ConcreteAA AA) noexcept
      : AA(getOpaquePtr(AA)), VT(&VtableFor<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }

  constexpr explicit AliasIteratorRef(void *AA, const VTable *VT) noexcept
      : AA(AA), VT(VT) {
    assert(AA != nullptr);
    assert(VT != nullptr);
  }

  constexpr AliasIteratorRef(const AliasIteratorRef &) noexcept = default;
  constexpr AliasIteratorRef &
  operator=(const AliasIteratorRef &) noexcept = default;
  ~AliasIteratorRef() = default;

  /// \brief Invokes the callback WithAlias for all aliases of Of at the
  /// instruction At.
  ///
  /// Note: The alias-relation is reflexive, so WithAlias is also called with
  /// Of.
  ///
  /// \param Of The pointer, for which the aliases should be iterated
  /// \param At The instruction, where the alias-query is raised.
  /// Implementations may ignore this parameter
  /// \param WithAlias Callback to invoke for each alias of Of
  void aliasesOf(ByConstRef<v_t> Of, ByConstRef<n_t> At,
                 llvm::function_ref<void(v_t)> WithAlias) {
    assert(VT != nullptr);
    VT->AliasesOf(AA, Of, At, WithAlias);
  }

  /// \brief Convenience function to aggregate all aliases of Of in a set.
  ///
  /// \param Of The pointer, for which the aliases should be iterated
  /// \param At The instruction, where the alias-query is raised.
  /// Implementations may ignore this parameter
  /// \tparam SetT The set-type of the set to create
  /// \returns A set of type SetT containing all aliases of Of
  template <typename SetT = std::set<v_t>>
  [[nodiscard]] SetT asSet(ByConstRef<v_t> Of, ByConstRef<n_t> At) {
    SetT Set;
    aliasesOf(Of, At, [&Set](v_t Alias) { Set.insert(std::move(Alias)); });
    return Set;
  }

  /// \brief Checks, whether Ptr and Alias may/must/partial/no-alias at
  /// instruction At.
  ///
  /// \param Ptr The pointer, for which the aliases should be iterated
  /// \param Alias A pointer, which may be a potential alias of Ptr
  /// \param At The instruction, where the alias-query is raised.
  [[nodiscard]] AliasResult alias(ByConstRef<v_t> Ptr, ByConstRef<v_t> Alias,
                                  ByConstRef<n_t> At) {
    assert(VT != nullptr);
    return VT->Alias(AA, Ptr, Alias, At);
  }

private:
  template <typename ConcreteAA>
  static void aliasesOfThunk(void *AA, ByConstRef<v_t> Of, ByConstRef<n_t> At,
                             llvm::function_ref<void(v_t)> WithAlias) {
    auto *CAA = fromOpaquePtr<ConcreteAA>(AA);
    if constexpr (IsAliasIterator<ConcreteAA>) {
      return (void)CAA->aliasesof(Of, At, WithAlias);
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

    auto *CAA = fromOpaquePtr<ConcreteAA>(AA);
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

} // namespace psr

#endif // PHASAR_POINTER_ALIASITERATOR_H
