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
#include <concepts>
#include <type_traits>

namespace psr {

namespace detail {

template <typename T, typename V, typename N>
concept IsAliasIteratorFor =
    requires(T &AI, V Ptr, N Inst, llvm::function_ref<void(V)> Callback) {
      AI.forallAliasesOf(Ptr, Inst, Callback);
    };

template <typename T>
concept HasAlias = requires(T &AS, typename T::v_t Ptr, typename T::n_t Inst) {
  { AS.alias(Ptr, Ptr, Inst) } -> std::convertible_to<AliasResult>;
};

template <typename T>
concept HasGetAliasSet =
    requires(T &AS, typename T::v_t Ptr, typename T::n_t Inst) {
      AS.getAliasSet(Ptr, Inst);
    };

} // namespace detail
template <typename T>
concept IsAliasIterator = requires {
  typename T::v_t;
  typename T::n_t;
  requires detail::IsAliasIteratorFor<T, typename T::v_t, typename T::n_t>;
};

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

  using ForallAliasesOfFn = void (*)(void *, ByConstRef<V>, ByConstRef<N>,
                                     llvm::function_ref<void(V)>);

  template <typename ConcreteAA>
    requires(!std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
             (detail::IsAliasIteratorFor<ConcreteAA, V, N> ||
              detail::HasGetAliasSet<ConcreteAA>))
  constexpr AliasIteratorRef(ConcreteAA *AA) noexcept
      : AA(getOpaquePtr(psr::assertNotNull(AA))), Fn(TypeErase<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }
  template <typename ConcreteAA>
    requires(!std::is_base_of_v<AliasIteratorRef, ConcreteAA> &&
             (detail::IsAliasIteratorFor<ConcreteAA, V, N> ||
              detail::HasGetAliasSet<ConcreteAA>) &&
             CanSSO<ConcreteAA>)
  constexpr AliasIteratorRef(ConcreteAA AA) noexcept
      : AA(getOpaquePtr(AA)), Fn(TypeErase<ConcreteAA>) {
    static_assert(IsAliasIterator<AliasIteratorRef>);
  }

  constexpr explicit AliasIteratorRef(void *AA, ForallAliasesOfFn Fn) noexcept
      : AA(AA), Fn(Fn) {
    assert(Fn != nullptr);
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
  void forallAliasesOf(ByConstRef<v_t> Of, ByConstRef<n_t> At,
                       llvm::function_ref<void(v_t)> WithAlias) {
    assert(Fn != nullptr);
    Fn(AA, Of, At, WithAlias);
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
    forallAliasesOf(Of, At,
                    [&Set](v_t Alias) { Set.insert(std::move(Alias)); });
    return Set;
  }

private:
  template <typename ConcreteAA>
  static void aliasesOfThunk(void *AA, ByConstRef<v_t> Of, ByConstRef<n_t> At,
                             llvm::function_ref<void(v_t)> WithAlias) {
    auto *CAA = fromOpaquePtr<ConcreteAA>(AA);
    if constexpr (detail::IsAliasIteratorFor<ConcreteAA, V, N>) {
      return (void)CAA->forallAliasesOf(Of, At, WithAlias);
    } else {
      auto AliasSetPtr = CAA->getAliasSet(Of, At);
      for (auto &&Alias : *AliasSetPtr) {
        WithAlias(PSR_FWD(Alias));
      }
    }
  }

  template <typename ConcreteAA>
  static constexpr ForallAliasesOfFn TypeErase = &aliasesOfThunk<ConcreteAA>;

  void *AA{};
  ForallAliasesOfFn Fn{};
}; // namespace psr

} // namespace psr

#endif // PHASAR_POINTER_ALIASITERATOR_H
