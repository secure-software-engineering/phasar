/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTION_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TypeName.h"

#include <atomic>
#include <tuple>
#include <type_traits>

namespace psr {

template <typename L> class EdgeFunction;
template <typename EF> class EdgeFunctionRef;

#if __cplusplus < 202002L

namespace detail {
template <typename T, typename = void>
struct IsEdgeFunction : std::false_type {};

template <typename T>
struct IsEdgeFunction<
    T, std::void_t<
           typename T::l_t,
           decltype(std::declval<const T &>().computeTarget(
               std::declval<typename T::l_t>())),
           decltype(T::compose(std::declval<EdgeFunctionRef<T>>(),
                               std::declval<EdgeFunction<typename T::l_t>>())),
           decltype(T::join(std::declval<EdgeFunctionRef<T>>(),
                            std::declval<EdgeFunction<typename T::l_t>>()))>>
    : std::true_type {};
} // namespace detail
template <typename T>
static inline constexpr bool IsEdgeFunction = detail::IsEdgeFunction<T>::value;
#else
// clang-format off
template <typename T>
concept IsEdgeFunction = requires(const T &EF, const EdgeFunction<typename T::l_t>& TEEF, EdgeFunctionRef<T> CEF, typename T::l_t Src) {
  typename T::l_t;
  {EF.computeTarget(Src)}   -> std::convertible_to<typename T::l_t>;
  {T::compose(CEF, TEEF)}  -> std::convertible_to<EdgeFunction<typename T::l_t>>;
  {T::join(CEF, TEEF)}     -> std::convertible_to<EdgeFunction<typename T::l_t>>;
};
// clang-format on
#endif

class EdgeFunctionBase {
public:
  template <typename ConcreteEF>
  static constexpr bool
      IsSOOCandidate = sizeof(ConcreteEF) <= sizeof(void *) && // NOLINT
                       alignof(ConcreteEF) <= alignof(void *) &&
                       std::is_trivially_copyable_v<ConcreteEF>;

protected:
  struct RefCountedBase {
    mutable std::atomic_size_t Rc = 0;
  };
  template <typename T> struct RefCounted : RefCountedBase { T Value; };

  template <typename ConcreteEF>
  constexpr static inline const ConcreteEF *
  getPtr(const void *const &EF) noexcept {
    if constexpr (!IsSOOCandidate<ConcreteEF>) {
      return &static_cast<const RefCounted<ConcreteEF> *>(EF)->Value;
    } else {
      return static_cast<const ConcreteEF *>(static_cast<const void *>(&EF));
    }
  }
  template <typename ConcreteEF>
  constexpr static inline const ConcreteEF *
  getPtr(const void *const &&EF) = delete; // NOLINT
};

/// Non-null reference to an edge function that is guarenteed to be managed by a
/// EdgeFunction object.
template <typename EF>
class [[clang::trivial_abi]] EdgeFunctionRef final : EdgeFunctionBase {
  template <typename L> friend class EdgeFunction;

public:
  using l_t = typename EF::l_t;

  EdgeFunctionRef(const EdgeFunctionRef &) noexcept = default;
  EdgeFunctionRef &operator=(const EdgeFunctionRef &) noexcept = default;
  ~EdgeFunctionRef() = default;

  const EF *operator->() const noexcept { return getPtr<EF>(Instance); }
  const EF *get() const noexcept { return getPtr<EF>(Instance); }
  const EF &operator*() const noexcept { return *getPtr<EF>(Instance); }

private:
  explicit EdgeFunctionRef(const void *Instance) noexcept
      : Instance(Instance) {}
  const void *Instance{};
};

/// Ref-counted and type-erased edge function with small-object optimization
template <typename L>
// -- combined copy and move assignment
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class [[clang::trivial_abi]] EdgeFunction final : EdgeFunctionBase {
public:
  using l_t = L;

  // --- Constructors
  EdgeFunction() noexcept = default;
  EdgeFunction(std::nullptr_t) noexcept : EdgeFunction() {}
  EdgeFunction(const EdgeFunction &Other) noexcept
      : EdgeFunction(Other.EF, Other.VTAndHeapAlloc) {}
  EdgeFunction(EdgeFunction &&Other) noexcept
      : EF(std::exchange(Other.EF, nullptr)),
        VTAndHeapAlloc(
            std::exchange(Other.VTAndHeapAlloc, decltype(VTAndHeapAlloc){})) {}
  void swap(EdgeFunction &Other) noexcept {
    std::swap(EF, Other.EF);
    std::swap(VTAndHeapAlloc, Other.VTAndHeapAlloc);
  }
  friend void swap(EdgeFunction &LHS, EdgeFunction &RHS) noexcept {
    LHS.swap(RHS);
  }

  EdgeFunction &operator=(EdgeFunction Other) noexcept {
    std::destroy_at(this);
    return *new (this) EdgeFunction(std::move(Other)); // NOLINT
  }
  EdgeFunction &operator=(std::nullptr_t) noexcept {
    std::destroy_at(this);
    return *new (this) EdgeFunction(); // NOLINT
  }

  ~EdgeFunction() noexcept {
    if (VTAndHeapAlloc.getInt()) {
      assert(VTAndHeapAlloc.getPointer() != nullptr && "Heap-alloc'd nullptr?");
      // Note: Memory-order taken from llvm::ThreadSafeRefCountedBase
      if (static_cast<const RefCountedBase *>(EF)->Rc.fetch_sub(
              1, std::memory_order_acq_rel) == 1) {
        VTAndHeapAlloc.getPointer()->destroy(EF);
      }
    }
  }

  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  EdgeFunction(EdgeFunctionRef<ConcreteEF> CEF) noexcept
      : EdgeFunction(CEF.Instance,
                     {&VTableFor<ConcreteEF>, !IsSOOCandidate<ConcreteEF>}) {}

  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  EdgeFunction(ConcreteEF &&CEF) noexcept(
      IsSOOCandidate<std::decay_t<ConcreteEF>>)
      : EdgeFunction(std::in_place_type<std::decay_t<ConcreteEF>>,
                     std::forward<ConcreteEF>(CEF)) {}

  template <typename ConcreteEF, typename... ArgTys>
  explicit EdgeFunction(
      std::in_place_type_t<ConcreteEF> /*unused*/,
      ArgTys &&...Args) noexcept(IsSOOCandidate<std::decay_t<ConcreteEF>> &&
                                     std::is_nothrow_constructible_v<ConcreteEF,
                                                                     ArgTys...>)
      : EdgeFunction(
            [](auto &&...Args) {
              if constexpr (IsSOOCandidate<std::decay_t<ConcreteEF>>) {
                void *Ret;
                new (&Ret) ConcreteEF(std::forward<ArgTys>(Args)...);
                return Ret;
              } else {
                return new RefCounted<ConcreteEF>{
                    {}, {std::forward<ArgTys>(Args)...}};
              }
            }(std::forward<ArgTys>(Args)...),
            {&VTableFor<ConcreteEF>, !IsSOOCandidate<ConcreteEF>}) {
    static_assert(std::is_same_v<l_t, typename ConcreteEF::l_t>,
                  "Cannot construct EdgeFunction with incompatible "
                  "lattice domain");
  }

  // ---  API functions

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    assert(!!*this && "computeTarget() called on nullptr!");
    return VTAndHeapAlloc.getPointer()->computeTarget(EF, std::move(Source));
  }

  [[nodiscard]] EdgeFunction composeWith(const EdgeFunction &SecondEF) const {
    return compose(*this, SecondEF);
  }

  [[nodiscard]] static EdgeFunction compose(const EdgeFunction &FirstEF,
                                            const EdgeFunction &SecondEF) {
    assert(!!FirstEF && "compose() called on LHS nullptr!");
    assert(!!SecondEF && "compose() called on RHS nullptr!");
    return FirstEF.VTAndHeapAlloc.getPointer()->compose(FirstEF.EF, SecondEF);
  }

  [[nodiscard]] EdgeFunction joinWith(const EdgeFunction &OtherEF) const {
    return join(*this, OtherEF);
  }

  [[nodiscard]] static EdgeFunction join(const EdgeFunction &FirstEF,
                                         const EdgeFunction &SecondEF) {
    assert(!!FirstEF && "join() called on LHS nullptr!");
    assert(!!SecondEF && "join() called on RHS nullptr!");
    return FirstEF.VTAndHeapAlloc.getPointer()->join(FirstEF.EF, SecondEF);
  }

  [[nodiscard]] friend bool operator==(const EdgeFunction &LHS,
                                       const EdgeFunction &RHS) noexcept {
    if (LHS.VTAndHeapAlloc != RHS.VTAndHeapAlloc) {
      return false;
    }
    if (LHS.VTAndHeapAlloc.getOpaqueValue() == nullptr) {
      return true;
    }
    return LHS.EF == RHS.EF ||
           LHS.VTAndHeapAlloc.getPointer()->equals(LHS.EF, RHS.EF);
  }

  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  [[nodiscard]] friend bool operator==(EdgeFunctionRef<ConcreteEF> LHS,
                                       const EdgeFunction &RHS) noexcept {
    if (!RHS.template isa<ConcreteEF>()) {
      return false;
    }
    if (LHS.Instance == RHS.EF) {
      return true;
    }
    if constexpr (IsEqualityComparable<ConcreteEF>) {
      return *LHS == *getPtr<ConcreteEF>(RHS.EF);
    } else {
      return true;
    }
  }

  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  [[nodiscard]] friend bool
  operator==(const EdgeFunction<L> &LHS,
             EdgeFunctionRef<ConcreteEF> RHS) noexcept {
    return RHS == LHS;
  }
  [[nodiscard]] friend bool operator==(const EdgeFunction &EF,
                                       std::nullptr_t) noexcept {
    return EF.VTAndHeapAlloc.getOpaqueValue() == nullptr;
  }
  [[nodiscard]] friend bool operator==(std::nullptr_t,
                                       const EdgeFunction &EF) noexcept {
    return EF.VTAndHeapAlloc.getOpaqueValue() == nullptr;
  }
  [[nodiscard]] friend bool operator!=(const EdgeFunction &LHS,
                                       const EdgeFunction &RHS) noexcept {
    return !(LHS == RHS);
  }
  [[nodiscard]] friend bool operator!=(const EdgeFunction &EF,
                                       std::nullptr_t) noexcept {
    return !(EF == nullptr);
  }
  [[nodiscard]] friend bool operator!=(std::nullptr_t,
                                       const EdgeFunction &EF) noexcept {
    return !(EF == nullptr);
  }

  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  [[nodiscard]] friend bool operator!=(EdgeFunctionRef<ConcreteEF> LHS,
                                       const EdgeFunction<L> &RHS) noexcept {
    return !(LHS == RHS);
  }
  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  [[nodiscard]] friend bool
  operator!=(const EdgeFunction<L> &LHS,
             EdgeFunctionRef<ConcreteEF> RHS) noexcept {
    return !(LHS == RHS);
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeFunction &EF) {
    if (!EF) {
      return OS << "<null-EF>";
    }

    EF.VTAndHeapAlloc.getPointer()->print(EF.EF, OS);
    return OS;
  }

  /// Arbitrary partial ordering for being able to sort edge
  /// functions.
  [[nodiscard]] friend bool operator<(const EdgeFunction &LHS,
                                      const EdgeFunction &RHS) noexcept {
    if (LHS == RHS) {
      return false;
    }
    return std::tuple(LHS.EF, LHS.VTAndHeapAlloc.getOpaqueValue()) <
           std::tuple(RHS.EF, RHS.VTAndHeapAlloc.getOpaqueValue());
  }

  [[nodiscard]] bool isConstant() const noexcept {
    assert(!!*this && "isConstant() called on nullptr!");
    return VTAndHeapAlloc.getPointer()->isConstant(EF);
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return VTAndHeapAlloc.getOpaqueValue();
  }

  template <typename ConcreteEF> [[nodiscard]] bool isa() const noexcept {
    if constexpr (IsEdgeFunction<ConcreteEF> &&
                  std::is_same_v<l_t, typename ConcreteEF::l_t>) {
      return VTAndHeapAlloc.getPointer() == &VTableFor<ConcreteEF>;
    } else {
      return false;
    }
  }

  template <typename ConcreteEF>
  [[nodiscard]] const ConcreteEF *cast() const noexcept {
    assert(this->template isa<ConcreteEF>() && "Cast on incompatible type!");
    return getPtr<ConcreteEF>(EF);
  }

  template <typename ConcreteEF>
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] const ConcreteEF *dyn_cast() const noexcept {
    return this->template isa<ConcreteEF>() ? getPtr<ConcreteEF>(EF) : nullptr;
  }

  // -- misc

  [[nodiscard]] bool isRefCounted() const noexcept {
    return VTAndHeapAlloc.getInt();
  }

private:
  struct VTable {
    // NOLINTBEGIN(readability-identifier-naming)
    l_t (*computeTarget)(const void *, l_t);
    EdgeFunction (*compose)(const void *, const EdgeFunction &);
    EdgeFunction (*join)(const void *, const EdgeFunction &);
    bool (*equals)(const void *, const void *) noexcept;
    void (*print)(const void *, llvm::raw_ostream &);
    bool (*isConstant)(const void *) noexcept;
    void (*destroy)(const void *) noexcept;
    // NOLINTEND(readability-identifier-naming)
  };

  template <typename ConcreteEF>
  static constexpr VTable VTableFor = {
      [](const void *EF, l_t Source) {
        return getPtr<ConcreteEF>(EF)->computeTarget(std::move(Source));
      },
      [](const void *EF, const EdgeFunction &SecondEF) {
        return ConcreteEF::compose(EdgeFunctionRef<ConcreteEF>(EF), SecondEF);
      },
      [](const void *EF, const EdgeFunction &OtherEF) {
        return ConcreteEF::join(EdgeFunctionRef<ConcreteEF>(EF), OtherEF);
      },
      [](const void *EF1, const void *EF2) noexcept {
        static_assert(IsEqualityComparable<ConcreteEF> ||
                          std::is_empty_v<ConcreteEF>,
                      "An EdgeFunction must be equality comparable with "
                      "operator==. Only if the type is empty, i.e. has no "
                      "members, the comparison can be inferred.");
        if constexpr (IsEqualityComparable<ConcreteEF>) {
          return *getPtr<ConcreteEF>(EF1) == *getPtr<ConcreteEF>(EF2);
        } else {
          return true;
        }
      },
      [](const void *EF, llvm::raw_ostream &OS) {
        if constexpr (is_llvm_printable_v<ConcreteEF>) {
          OS << *getPtr<ConcreteEF>(EF);
        } else {
          OS << llvm::getTypeName<ConcreteEF>();
        }
      },
      [](const void *EF) noexcept {
        if constexpr (HasIsConstant<ConcreteEF>) {
          static_assert(
              std::is_nothrow_invocable_v<decltype(&ConcreteEF::isConstant),
                                          const ConcreteEF &>,
              "The function isConstant() must be noexcept!");
          return getPtr<ConcreteEF>(EF)->isConstant();
        } else {
          return false;
        }
      },
      [](const void *EF) noexcept {
        if constexpr (!IsSOOCandidate<ConcreteEF>) {
          delete static_cast<const RefCounted<ConcreteEF> *>(EF);
        }
      },
  };

  // Utility ctor for (copy) construction. Increments the ref-count if
  // necessary
  explicit EdgeFunction(
      const void *EF,
      llvm::PointerIntPair<const VTable *, 1, bool> VTAndHeapAlloc) noexcept
      : EF(EF), VTAndHeapAlloc(VTAndHeapAlloc) {
    if (VTAndHeapAlloc.getInt()) {
      // Note: Memory-order taken from llvm::ThreadSafeRefCountedBase
      static_cast<const RefCountedBase *>(EF)->Rc.fetch_add(
          1, std::memory_order_relaxed);
    }
  }

  // -- data members

  const void *EF{};
  llvm::PointerIntPair<const VTable *, 1, bool> VTAndHeapAlloc{};
};

} // namespace psr

namespace llvm {

// LLVM is currently overhauling its casting system. Use the new variant once
// possible!
// Note: The new variant (With CastInfo) is not tested yet!
#if LLVM_MAJOR < 15

template <typename To, typename L>
struct isa_impl_cl<To, const psr::EdgeFunction<L>> {
  static inline bool doit(const psr::EdgeFunction<L> &Val) noexcept {
    assert(Val && "isa<> used on a null pointer");
    return Val.template isa<std::decay_t<To>>();
  }
};

template <typename To, typename L>
struct cast_retty_impl<To, const psr::EdgeFunction<L>> {
  using ret_type = const To *;
};

template <typename To, typename L>
struct cast_retty_impl<To, psr::EdgeFunction<L>>
    : cast_retty_impl<To, const psr::EdgeFunction<L>> {};

template <class To, class L>
struct cast_convert_val<To, const psr::EdgeFunction<L>,
                        const psr::EdgeFunction<L>> {
  static typename cast_retty<To, psr::EdgeFunction<L>>::ret_type
  doit(const psr::EdgeFunction<L> &Val) noexcept {
    return Val.template cast<To>();
  }
};
template <class To, class L>
struct cast_convert_val<To, psr::EdgeFunction<L>, psr::EdgeFunction<L>>
    : cast_convert_val<To, const psr::EdgeFunction<L>,
                       const psr::EdgeFunction<L>> {};

template <typename To, typename L>
[[nodiscard]] inline typename cast_retty<To, psr::EdgeFunction<L>>::ret_type
dyn_cast_or_null(const psr::EdgeFunction<L> &EF) noexcept { // NOLINT
  return (EF && isa<To>(EF)) ? cast<To>(EF) : nullptr;
}

template <typename To, typename L>
[[nodiscard]] inline typename cast_retty<To, psr::EdgeFunction<L>>::ret_type
cast_or_null(const psr::EdgeFunction<L> &EF) noexcept { // NOLINT
  return EF ? cast<To>(EF) : nullptr;
}
#else

template <typename To, typename L>
struct CastIsPossible<To, psr::EdgeFunction<L>> {
  static inline bool isPossible(const psr::EdgeFunction<L> &EF) noexcept {
    return EF->template isa<To>();
  }
};

template <typename To, typename L>
struct CastInfo<To, psr::EdgeFunction<L>>
    : public CastIsPossible<To, psr::EdgeFunction<L>>,
      public NullableValueCastFailed<const To *>,
      public DefaultDoCastIfPossible<const To *, const psr::EdgeFunction<L> &,
                                     CastInfo<To, psr::EdgeFunction<L>>> {
  static inline const To *doCast(const psr::EdgeFunction<L> &EF) noexcept {
    return EF.template cast<To>();
  }
};

template <typename To, typename L>
struct CastInfo<To, const psr::EdgeFunction<L>>
    : public ConstStrippingForwardingCast<To, const psr::EdgeFunction<L>,
                                          CastInfo<To, psr::EdgeFunction<L>>> {
};

#endif
} // namespace llvm

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTION_H
