/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_POINTSTOINFOBASE_H
#define PHASAR_POINTER_POINTSTOINFOBASE_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include <optional>
#include <type_traits>

namespace psr {
template <typename T> struct PointsToTraits {
  // using v_t              -- Type of pointers
  // using n_t              -- Type of instructions (ICFG-nodes)
  // using o_t              -- Type of abstract objects (elements of the
  //                           points-to sets)
  // using PointsToSetTy    -- Type of a points-to set (value-type)
  // using PointsToSetPtrTy -- Type of a pointer to a points-to set
  //                           (not necessarily PointsToSetTy const *)
};

template <typename T, typename Enable = void>
struct is_PointsToTraits : std::false_type {}; // NOLINT
template <typename T>
struct is_PointsToTraits<
    T, std::void_t<typename T::v_t, typename T::n_t, typename T::o_t,
                   typename T::PointsToSetTy, typename T::PointsToSetPtrTy>>
    : std::true_type {};

template <typename T>
static constexpr bool is_PointsToTraits_v = // NOLINT
    is_PointsToTraits<T>::value;

// clang-format off
template <typename T1, typename T2>
static constexpr bool is_equivalent_PointsToTraits_v = // NOLINT
    is_PointsToTraits_v<T1> && is_PointsToTraits_v<T2> &&
    std::is_same_v<typename T1::n_t, typename T2::n_t> &&
    std::is_same_v<typename T1::v_t, typename T2::v_t> &&
    std::is_same_v<typename T1::o_t, typename T2::o_t> &&
    std::is_same_v<typename T1::PointsToSetPtrTy,
                   typename T2::PointsToSetPtrTy>;
// clang-format on

/// Base class of all points-to analysis implementations. Don't use this class
/// directly. For a type-erased variant, use PointsToInfoRef or PointsToInfo.
template <typename Derived> class PointsToInfoBase {
public:
  using v_t = typename PointsToTraits<Derived>::v_t;
  using n_t = typename PointsToTraits<Derived>::n_t;
  using o_t = typename PointsToTraits<Derived>::o_t;
  using PointsToSetTy = typename PointsToTraits<Derived>::PointsToSetTy;
  using PointsToSetPtrTy = typename PointsToTraits<Derived>::PointsToSetPtrTy;

  explicit PointsToInfoBase() noexcept {
    static_assert(std::is_base_of_v<PointsToInfoBase, Derived>,
                  "Invalid CRTP instantiation: Derived must inherit from "
                  "PointsToInfoBase<Derived>!");
  }

  /// Creates an abstract object corresponding to the given pointer
  [[nodiscard]] o_t asAbstractObject(ByConstRef<v_t> Pointer) const noexcept {
    return self().asAbstractObjectImpl(Pointer);
  }

  /// Inverse function of asAbstractObject. If Obj does not directly correspond
  /// to a pointer, this function may return nullopt.
  [[nodiscard]] std::optional<v_t>
  asPointerOrNull(ByConstRef<o_t> Obj) const noexcept {
    return self().asPointerOrNullImpl(Obj);
  }

  [[nodiscard]] bool mayPointsTo(ByConstRef<o_t> Pointer, ByConstRef<o_t> Obj,
                                 ByConstRef<n_t> AtInstruction) const {
    return self().mayPointsToImpl(Pointer, Obj, AtInstruction);
  }

  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] bool mayPointsTo(ByConstRef<v_t> Pointer, ByConstRef<o_t> Obj,
                                 ByConstRef<n_t> AtInstruction) const {
    return self().mayPointsToImpl(Pointer, Obj, AtInstruction);
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSet(ByConstRef<o_t> Pointer, ByConstRef<n_t> AtInstruction) const {
    return self().getPointsToSetImpl(Pointer, AtInstruction);
  }

  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] PointsToSetPtrTy
  getPointsToSet(ByConstRef<v_t> Pointer, ByConstRef<n_t> AtInstruction) const {
    return self().getPointsToSetImpl(Pointer, AtInstruction);
  }

  /// Gets all pointers v_t where we have non-empty points-to information at
  /// this instruction
  [[nodiscard]] decltype(auto)
  getInterestingPointersAt(ByConstRef<n_t> AtInstruction) const {
    static_assert(
        is_iterable_over_v<
            decltype(self().getInterestingPointersAtImpl(AtInstruction)), v_t>);
    return self().getInterestingPointersAtImpl(AtInstruction);
  }

private:
  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] bool mayPointsToImpl(ByConstRef<v_t> Pointer,
                                     ByConstRef<o_t> Obj,
                                     ByConstRef<n_t> AtInstruction) const {
    return getPointsToSet(asAbstractObject(Pointer), AtInstruction)->count(Obj);
  }

  [[nodiscard]] bool mayPointsToImpl(ByConstRef<o_t> Pointer,
                                     ByConstRef<o_t> Obj,
                                     ByConstRef<n_t> AtInstruction) const {
    return getPointsToSet(Pointer, AtInstruction)->count(Obj);
  }

  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<v_t> Pointer,
                     ByConstRef<n_t> AtInstruction) const {
    return self().getPointsToSetImpl(asAbstractObject(Pointer), AtInstruction);
  }

  [[nodiscard]] Derived &self() noexcept {
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

} // namespace psr

#endif // PHASAR_POINTER_POINTSTOINFOBASE_H
