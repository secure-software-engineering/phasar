/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H
#define PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H

#include "phasar/PhasarLLVM/Pointer/PointsToInfoBase.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace psr {

template <typename PTATraits, typename = void> class PointsToInfoRef;
template <typename PTATraits, typename = void> class PointsToInfo;

template <typename PTATraits>
struct PointsToTraits<PointsToInfoRef<PTATraits>> : PTATraits {};
template <typename PTATraits>
struct PointsToTraits<PointsToInfo<PTATraits>> : PTATraits {};

template <typename PTATraits>
class PointsToInfoRef<PTATraits,
                      std::enable_if_t<is_PointsToTraits_v<PTATraits>>>
    : PointsToInfoBase<PointsToInfoRef<PTATraits>> {
  friend class PointsToInfo<PTATraits>;
  using base_t = PointsToInfoBase<PointsToInfoRef<PTATraits>>;

public:
  using typename base_t::n_t;
  using typename base_t::o_t;
  using typename base_t::PointsToSetPtrTy;
  using typename base_t::PointsToSetTy;
  using typename base_t::v_t;

  PointsToInfoRef() noexcept = default;
  PointsToInfoRef(std::nullptr_t) noexcept : PointsToInfoRef() {}

  template <typename ConcretePTA,
            typename = std::enable_if_t<
                !std::is_base_of_v<PointsToInfoRef, ConcretePTA> &&
                is_equivalent_PointsToTraits_v<PTATraits,
                                               PointsToTraits<ConcretePTA>>>>
  PointsToInfoRef(const ConcretePTA *PT) noexcept
      : PT(PT), VT(&VTableFor<ConcretePTA>) {
    if constexpr (!std::is_empty_v<ConcretePTA>) {
      assert(PT != nullptr);
    }
  }

  PointsToInfoRef(const PointsToInfoRef &) noexcept = default;
  PointsToInfoRef &operator=(const PointsToInfoRef &) noexcept = default;
  ~PointsToInfoRef() noexcept = default;

  explicit operator bool() const noexcept { return VT != nullptr; }

private:
  struct VTableBase {
    o_t (*AsAbstractObject)(const void *, ByConstRef<v_t>) noexcept;
    bool (*MayPointsTo)(const void *, ByConstRef<o_t>, ByConstRef<o_t>,
                        ByConstRef<n_t>);
    PointsToSetPtrTy (*GetPointsToSet)(const void *, ByConstRef<o_t>,
                                       ByConstRef<n_t>);
    void (*Destroy)(void *); // Useful for the owning variant
  };

  template <typename V = v_t, typename = void> struct VTable : VTableBase {};
  template <typename V>
  struct VTable<V, std::enable_if_t<!std::is_same_v<o_t, V>>> : VTableBase {
    bool (*MayPointsToV)(const void *, ByConstRef<v_t>, ByConstRef<o_t>,
                         ByConstRef<n_t>);
    PointsToSetPtrTy (*GetPointsToSetV)(const void *, ByConstRef<v_t>,
                                        ByConstRef<n_t>);
  };

  template <typename ConcretePTA>
  constexpr static VTable<> makeVTableFor() noexcept {
    constexpr VTableBase Base = {
        [](const void *PT, ByConstRef<v_t> Pointer) noexcept {
          return static_cast<const ConcretePTA *>(PT)->asAbstractObject(
              Pointer);
        },
        [](const void *PT, ByConstRef<o_t> Pointer, ByConstRef<o_t> Obj,
           ByConstRef<n_t> AtInstruction) {
          return static_cast<const ConcretePTA *>(PT)->mayPointsTo(
              Pointer, Obj, AtInstruction);
        },
        [](const void *PT, ByConstRef<o_t> Pointer,
           ByConstRef<n_t> AtInstruction) {
          return static_cast<const ConcretePTA *>(PT)->getPointsToSet(
              Pointer, AtInstruction);
        },
        [](void *PT) { delete static_cast<ConcretePTA *>(PT); },
    };
    if constexpr (std::is_same_v<o_t, v_t>) {
      return {Base};
    } else {
      return {
          Base,
          [](const void *PT, ByConstRef<v_t> Pointer, ByConstRef<o_t> Obj,
             ByConstRef<n_t> AtInstruction) {
            return static_cast<const ConcretePTA *>(PT)->mayPointsTo(
                Pointer, Obj, AtInstruction);
          },
          [](const void *PT, ByConstRef<v_t> Pointer,
             ByConstRef<n_t> AtInstruction) {
            return static_cast<const ConcretePTA *>(PT)->getPointsToSet(
                Pointer, AtInstruction);
          },
      };
    }
  }

  template <typename ConcretePTA>
  static constexpr VTable<> VTableFor = makeVTableFor<ConcretePTA>();

  // --- Impl for PointsToInfoBase:

  [[nodiscard]] o_t
  asAbstractObjectImpl(ByConstRef<v_t> Pointer) const noexcept {
    assert(VT);
    return VT->AsAbstractObject(PT, Pointer);
  }

  [[nodiscard]] bool mayPointsToImpl(ByConstRef<o_t> Pointer,
                                     ByConstRef<o_t> Obj,
                                     ByConstRef<n_t> AtInstruction) const {
    assert(VT);
    return VT->MayPointsTo(PT, Pointer, Obj, AtInstruction);
  }

  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] bool mayPointsToImpl(ByConstRef<v_t> Pointer,
                                     ByConstRef<o_t> Obj,
                                     ByConstRef<n_t> AtInstruction) const {
    assert(VT);
    return VT->MayPointsToV(PT, Pointer, Obj, AtInstruction);
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<o_t> Pointer,
                     ByConstRef<n_t> AtInstruction) const {
    assert(VT);
    return VT->GetPointsToSet(PT, Pointer, AtInstruction);
  }

  template <typename V = v_t,
            typename = std::enable_if_t<!std::is_same_v<V, o_t>>>
  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<v_t> Pointer,
                     ByConstRef<n_t> AtInstruction) const {
    assert(VT);
    return VT->GetPointsToSetV(PT, Pointer, AtInstruction);
  }

  // ---
  const void *PT{};
  const VTable<> *VT{};
};

template <typename PTATraits>
class PointsToInfo<PTATraits, std::enable_if_t<is_PointsToTraits_v<PTATraits>>>
    final : public PointsToInfoRef<PTATraits> {
  using base_t = PointsToInfoRef<PTATraits>;

public:
  using typename base_t::n_t;
  using typename base_t::o_t;
  using typename base_t::PointsToSetPtrTy;
  using typename base_t::PointsToSetTy;
  using typename base_t::v_t;

  PointsToInfo() noexcept = default;
  PointsToInfo(std::nullptr_t) noexcept {};
  PointsToInfo(const PointsToInfo &) = delete;
  PointsToInfo &operator=(const PointsToInfo &) = delete;
  PointsToInfo(PointsToInfo &&Other) noexcept { swap(Other); }
  PointsToInfo &operator=(PointsToInfo &&Other) noexcept {
    auto Cpy{std::move(Other)};
    swap(Cpy);
    return *this;
  }

  void swap(PointsToInfo &Other) noexcept {
    std::swap(this->PT, Other.PT);
    std::swap(this->VT, Other.VT);
  }
  friend void swap(PointsToInfo &LHS, PointsToInfo &RHS) noexcept {
    LHS.swap(RHS);
  }

  template <typename ConcretePTA, typename... ArgTys>
  explicit PointsToInfo(std::in_place_type_t<ConcretePTA> /*unused*/,
                        ArgTys &&...Args)
      : PointsToInfoRef<PTATraits>(
            new ConcretePTA(std::forward<ArgTys>(Args)...)) {}

  ~PointsToInfo() noexcept {
    if (*this) {
      this->VT->Destroy(const_cast<void *>(this->PT)); // NOLINT
      this->VT = nullptr;
      this->PT = nullptr;
    }
  }
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H
