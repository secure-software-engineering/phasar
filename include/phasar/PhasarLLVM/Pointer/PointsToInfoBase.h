/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOINFOBASE_H
#define PHASAR_PHASARLLVM_POINTER_POINTSTOINFOBASE_H

#include "phasar/PhasarLLVM/Utils/ByRef.h"

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
struct is_PointsToTraits : std::false_type {};
template <typename T>
struct is_PointsToTraits<
    T, std::void_t<typename T::v_t, typename T::n_t, typename T::o_t,
                   typename T::PointsToSetTy, typename T::PointsToSetPtrTy>>
    : std::true_type {};

template <typename T>
static constexpr bool is_PointsToTraits_v = is_PointsToTraits<T>::value;

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

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOINFOBASE_H
