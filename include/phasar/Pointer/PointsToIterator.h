/******************************************************************************
 * Copyright (c) 2025 Fabian Schíebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_POINTSTOITERATOR_H
#define PHASAR_POINTER_POINTSTOITERATOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/PointerUtils.h"
#include "phasar/Utils/TypeErasureUtils.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <type_traits>
#include <utility>

namespace psr {
namespace detail {
template <typename T, typename = void>
struct IsPointsToIterator : std::false_type {};

template <typename T>
struct IsPointsToIterator<
    T, std::void_t<decltype(std::declval<const T &>().forallPointeesOf(
           std::declval<typename T::o_t>(), std::declval<typename T::n_t>(),
           std::declval<llvm::function_ref<void(typename T::o_t)>>()))>>
    : std::true_type {};

template <typename T, typename = bool>
struct HasMayPointsTo : std::false_type {};
template <typename T>
struct HasMayPointsTo<T, decltype(std::declval<const T &>().mayPointsTo(
                             std::declval<typename T::o_t>(),
                             std::declval<typename T::o_t>(),
                             std::declval<typename T::n_t>()))>
    : std::true_type {};

template <typename T, typename = void>
struct HasGetPointsToSet : std::false_type {};
template <typename T>
struct HasGetPointsToSet<T, decltype(std::declval<const T &>().getPointsToSet(
                                std::declval<typename T::o_t>(),
                                std::declval<typename T::n_t>()))>
    : std::true_type {};

template <typename T, typename = void>
struct HasAsAbstractObject : std::false_type {};
template <typename T>
struct HasAsAbstractObject<T,
                           decltype(std::declval<const T &>().asAbstractObject(
                               std::declval<typename T::v_t>()))>
    : std::true_type {};

template <typename T, typename = void>
struct HasReachableAllocationSites : std::false_type {};
template <typename T>
struct HasReachableAllocationSites<
    T, std::void_t<decltype(std::declval<T &>().getReachableAllocationSites(
                       std::declval<typename T::v_t>(), true,
                       std::declval<typename T::n_t>())),
                   decltype(std::declval<T &>().isInReachableAllocationSites(
                       std::declval<typename T::v_t>(),
                       std::declval<typename T::v_t>(), true,
                       std::declval<typename T::n_t>()))>> : std::true_type {};
} // namespace detail

template <typename T>
PSR_CONCEPT IsPointsToIterator = detail::IsPointsToIterator<T>::value;

/// A type-erased reference to any object implementing the IsPointsToIterator
/// interface. Use this, if your alias-aware analysis just needs
/// a minimal interface to work with points-to relations and does not require
/// the versatility of PointsToInfoRef.
///
/// This is a *non-owning* reference similar to std::string_view and
/// llvm::ArrayRef. Pass values of this type by value.
///
/// \note You can also convert an AliasInfoRef to PointsToIteratorRef. In this
/// case, it will use the getReachableAllocationSites() API.
template <typename V, typename O, typename N>
class [[gsl::Pointer]] PointsToIteratorRef : protected TypeErasureUtils {
public:
  using v_t = V;
  using o_t = O;
  using n_t = N;

  struct VTable {
    o_t (*AsAbstractObject)(const void *, ByConstRef<v_t>) noexcept;
    void (*ForallPointeesOf)(const void *, ByConstRef<o_t>, ByConstRef<n_t>,
                             llvm::function_ref<void(O)>);
    bool (*MayPointsTo)(const void *, ByConstRef<o_t>, ByConstRef<o_t>,
                        ByConstRef<n_t>);
    void (*Destroy)(const void *) noexcept; // Useful for the owning variant
  };

  template <
      typename ConcretePTA,
      std::enable_if_t<!std::is_base_of_v<PointsToIteratorRef, ConcretePTA> &&
                       std::is_same_v<v_t, typename ConcretePTA::v_t> &&
                       std::is_same_v<o_t, typename ConcretePTA::o_t> &&
                       std::is_same_v<n_t, typename ConcretePTA::n_t> &&
                       !detail::HasReachableAllocationSites<ConcretePTA>::value>
          * = nullptr>
  constexpr PointsToIteratorRef(const ConcretePTA *PT) noexcept
      : PT(getOpaquePtr(psr::assertNotNull(PT))), VT(&VTableFor<ConcretePTA>) {
    static_assert(IsPointsToIterator<PointsToIteratorRef>);
  }

  template <
      typename ConcretePTA,
      std::enable_if_t<!std::is_base_of_v<PointsToIteratorRef, ConcretePTA> &&
                       std::is_same_v<v_t, typename ConcretePTA::v_t> &&
                       std::is_same_v<v_t, o_t> &&
                       std::is_same_v<n_t, typename ConcretePTA::n_t> &&
                       !std::is_const_v<ConcretePTA> && // Need non-const API
                       detail::HasReachableAllocationSites<ConcretePTA>::value>
          * = nullptr>
  constexpr PointsToIteratorRef(ConcretePTA *PT) noexcept
      : PT(getOpaquePtr(psr::assertNotNull(PT))),
        VT(&ReachableAllocSitesVTFor<ConcretePTA>) {
    static_assert(IsPointsToIterator<PointsToIteratorRef>);
  }

  template <typename ConcretePTA,
            typename = std::enable_if_t<
                !std::is_base_of_v<PointsToIteratorRef, ConcretePTA> &&
                std::is_same_v<v_t, typename ConcretePTA::v_t> &&
                std::is_same_v<o_t, typename ConcretePTA::o_t> &&
                std::is_same_v<n_t, typename ConcretePTA::n_t> &&
                CanSSO<ConcretePTA>>>
  constexpr PointsToIteratorRef(ConcretePTA PT) noexcept
      : PT(getOpaquePtr(PT)), VT(&VTableFor<ConcretePTA>) {
    static_assert(detail::IsPointsToIterator<PointsToIteratorRef>::value);
  }

  constexpr explicit PointsToIteratorRef(const void *PT,
                                         const VTable *VT) noexcept
      : PT(PT), VT(VT) {
    assert(VT != nullptr);
  }

  constexpr PointsToIteratorRef(const PointsToIteratorRef &) noexcept = default;
  constexpr PointsToIteratorRef &
  operator=(const PointsToIteratorRef &) noexcept = default;
  ~PointsToIteratorRef() = default;

  [[nodiscard]] o_t asAbstractObject(ByConstRef<v_t> Pointer) const noexcept {
    assert(VT != nullptr);
    return VT->AsAbstractObject(PT, Pointer);
  }

  void forallPointeesOf(ByConstRef<o_t> Pointer, ByConstRef<n_t> At,
                        llvm::function_ref<void(O)> WithPointee) const {
    assert(VT != nullptr);
    VT->ForallPointeesOf(PT, Pointer, At, WithPointee);
  }
  template <typename SetT = std::set<o_t>>
  [[nodiscard]] SetT asSet(ByConstRef<o_t> Pointer, ByConstRef<n_t> At) {
    SetT Set;
    forallPointeesOf(Pointer, At, [&Set](o_t Obj) { Set.insert(Obj); });
    return Set;
  }

  [[nodiscard]] bool mayPointsTo(ByConstRef<o_t> Pointer, ByConstRef<o_t> Obj,
                                 ByConstRef<n_t> At) const {
    assert(VT != nullptr);
    return VT->MayPointsTo(PT, Pointer, Obj, At);
  }

protected:
  constexpr PointsToIteratorRef() noexcept = default;

private:
  template <typename ConcretePTA>
  static o_t asAbstractObjectThunk(const void *PT,
                                   ByConstRef<v_t> Pointer) noexcept {
    if constexpr (detail::HasAsAbstractObject<ConcretePTA>::value) {
      return fromOpaquePtr<ConcretePTA>(PT)->asAbstractObject(Pointer);
    } else if constexpr (std::is_convertible_v<v_t, o_t>) {
      return Pointer;
    } else {
      static_assert(detail::HasAsAbstractObject<ConcretePTA>::value);
    }
  }

  template <typename ConcretePTA>
  static void forallPointeesOfThunk(const void *PT, ByConstRef<o_t> Pointer,
                                    ByConstRef<n_t> At,
                                    llvm::function_ref<void(O)> WithPointee) {
    const auto *CPT = fromOpaquePtr<ConcretePTA>(PT);
    if constexpr (IsPointsToIterator<ConcretePTA>) {
      return (void)CPT->forallPointeesOf(Pointer, At, WithPointee);
    } else {
      auto &&PointsToSet = CPT->getPointsToSet(Pointer, At);
      // The PointsToSet can be a set or a pointer to a set
      auto *PointsToSetPtr = getPointerFrom(PointsToSet);
      for (auto &&Pointee : *PointsToSetPtr) {
        WithPointee(PSR_FWD(Pointee));
      }
    }
  }

  template <typename ConcretePTA>
  static void
  forallReachableAllocationSitesThunk(const void *AS, ByConstRef<o_t> Pointer,
                                      ByConstRef<n_t> At,
                                      llvm::function_ref<void(O)> WithPointee) {
    auto *CAS = fromOpaquePtr<ConcretePTA>(AS);
    // Note: The getReachableAllocationSites() API requires non-const access to
    // support lazy computation (if requested in the LLVMAliasSet ctor).
    // The below should still be safe, as we restrict our PointsToInfoRef ctor
    // to take a non-const pointer to an aliasinfo.
    auto AliasSetPtr =
        const_cast<ConcretePTA *>(CAS)->getReachableAllocationSites(Pointer,
                                                                    true, At);

    for (auto &&Alias : *AliasSetPtr) {
      WithPointee(PSR_FWD(Alias));
    }
  }

  template <typename ConcretePTA>
  static bool mayPointsToThunk(const void *PT, ByConstRef<o_t> Pointer,
                               ByConstRef<o_t> Obj, ByConstRef<n_t> At) {
    const auto *CPT = fromOpaquePtr<ConcretePTA>(PT);
    if constexpr (detail::HasMayPointsTo<ConcretePTA>::value) {
      return CPT->mayPointsTo(Pointer, Obj, At);
    } else if constexpr (detail::HasGetPointsToSet<ConcretePTA>::value) {
      auto &&PointsToSet = CPT->getPointsToSet(Pointer, At);
      // The PointsToSet can be a set or a pointer to a set
      auto *PointsToSetPtr = getPointerFrom(PointsToSet);
      return PointsToSetPtr->count(Obj);
    } else {
      bool Ret = false;
      CPT->forallPointeesOf([&Ret, Obj](o_t Pointee) {
        if (Pointee == Obj) {
          Ret = true;
        }
      });
      return Ret;
    }
  }

  template <typename ConcretePTA>
  static bool maybeInReachableAllocationSitesThunk(const void *AS,
                                                   ByConstRef<o_t> Pointer,
                                                   ByConstRef<o_t> Obj,
                                                   ByConstRef<n_t> At) {
    if (Pointer == Obj) {
      return true;
    }

    auto *CAS = fromOpaquePtr<ConcretePTA>(AS);
    // Note: Same as for getReachableAllocationSites() above.
    return const_cast<ConcretePTA *>(CAS)->isInReachableAllocationSites(
        Pointer, Obj, true, At);
  }

  template <typename ConcretePTA>
  static void destroyThunk(const void *PT) noexcept {
    if constexpr (!CanSSO<ConcretePTA>) {
      delete fromOpaquePtr<ConcretePTA>(PT);
    }
  }

  template <typename ConcretePTA>
  static constexpr VTable VTableFor = {
      &asAbstractObjectThunk<ConcretePTA>,
      &forallPointeesOfThunk<ConcretePTA>,
      &mayPointsToThunk<ConcretePTA>,
      &destroyThunk<ConcretePTA>,
  };

  template <typename ConcreteAA>
  static constexpr VTable ReachableAllocSitesVTFor = {
      &asAbstractObjectThunk<ConcreteAA>,
      &forallReachableAllocationSitesThunk<ConcreteAA>,
      &maybeInReachableAllocationSitesThunk<ConcreteAA>,
      &destroyThunk<ConcreteAA>,
  };

protected:
  const void *PT{};
  const VTable *VT{};
};

/// Owning version of PointsToIteratorRef
template <typename V, typename O, typename N>
class [[clang::trivial_abi, gsl::Owner]] PointsToIterator
    : public PointsToIteratorRef<V, O, N> {
  using base_t = PointsToIteratorRef<V, O, N>;

public:
  using v_t = typename base_t::v_t;
  using o_t = typename base_t::o_t;
  using n_t = typename base_t::n_t;

  constexpr PointsToIterator() noexcept = default;
  constexpr PointsToIterator(std::nullptr_t) noexcept {};

  PointsToIterator(const PointsToIterator &) = delete;
  PointsToIterator &operator=(const PointsToIterator &) = delete;

  PointsToIterator(PointsToIterator &&Other) noexcept
      : base_t(std::exchange((base_t &)Other, {})) {}
  PointsToIterator &operator=(PointsToIterator &&Other) noexcept {
    PointsToIterator(std::move(Other)).swap(*this);
    return *this;
  }

  constexpr void swap(PointsToIterator &Other) noexcept {
    std::swap(this->PT, Other.PT);
    std::swap(this->VT, Other.VT);
  }
  constexpr friend void swap(PointsToIterator &LHS,
                             PointsToIterator &RHS) noexcept {
    LHS.swap(RHS);
  }

  template <typename ConcretePTA>
  constexpr PointsToIterator(std::unique_ptr<ConcretePTA> PTA)
      : base_t(PTA.get()) {
    if constexpr (!base_t::template CanSSO<ConcretePTA>) {
      PTA.release();
    }
  }

  template <typename ConcretePTA, typename... ArgTys,
            std::enable_if_t<!base_t::template CanSSO<ConcretePTA>>>
  constexpr explicit PointsToIterator(
      std::in_place_type_t<ConcretePTA> /*unused*/, ArgTys &&...Args)
      : PointsToIterator(
            std::make_unique<ConcretePTA>(std::forward<ArgTys>(Args)...)) {}

  template <typename ConcretePTA, typename... ArgTys,
            std::enable_if_t<base_t::template CanSSO<ConcretePTA>>>
  constexpr explicit PointsToIterator(
      std::in_place_type_t<ConcretePTA> /*unused*/, ArgTys &&...Args)
      : base_t([&] {
          if constexpr (std::is_aggregate_v<ConcretePTA>) {
            return ConcretePTA{std::forward<ArgTys>(Args)...};
          } else {
            return ConcretePTA(std::forward<ArgTys>(Args)...);
          }
        }()) {}

  ~PointsToIterator() noexcept {
    if (this->VT != nullptr) {
      this->VT->Destroy(this->PT);
      this->PT = nullptr;
      this->VT = nullptr;
    }
  }
};

} // namespace psr

#endif // PHASAR_POINTER_POINTSTOITERATOR_H
