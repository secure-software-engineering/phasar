/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTION_H
#define PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTION_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionSingletonCache.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TypeName.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <atomic>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

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
  {T::compose(CEF, TEEF)}  -> std::same_as<EdgeFunction<typename T::l_t>>;
  {T::join(CEF, TEEF)}     -> std::same_as<EdgeFunction<typename T::l_t>>;
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
  enum class AllocationPolicy {
    SmallObjectOptimized,
    DefaultHeapAllocated,
    CustomHeapAllocated,
  };
  struct RefCountedBase {
    mutable std::atomic_size_t Rc = 0;
  };
  template <typename T> struct RefCounted : RefCountedBase { T Value; };

  template <typename T> struct CachedRefCounted : RefCounted<T> {
    EdgeFunctionSingletonCache<T> *Cache{};
  };

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

  template <typename ConcreteEF>
  static constexpr AllocationPolicy DefaultAllocPolicy =
      IsSOOCandidate<ConcreteEF> ? AllocationPolicy::SmallObjectOptimized
                                 : AllocationPolicy::DefaultHeapAllocated;
  template <typename ConcreteEF>
  static constexpr AllocationPolicy CustomAllocPolicy =
      IsSOOCandidate<ConcreteEF> ? AllocationPolicy::SmallObjectOptimized
                                 : AllocationPolicy::CustomHeapAllocated;
};

/// Non-null reference to an edge function that is guarenteed to be managed by
/// an EdgeFunction object.
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

  [[nodiscard]] bool isCached() const noexcept {
    if constexpr (IsSOOCandidate<EF>) {
      return false;
    } else {
      return IsCached;
    }
  }

  [[nodiscard]] EdgeFunctionSingletonCache<EF> *
  getCacheOrNull() const noexcept {
    if (isCached()) {
      return static_cast<const CachedRefCounted<EF> *>(Instance)->Cache;
    }

    return nullptr;
  }

private:
  explicit EdgeFunctionRef(const void *Instance, bool IsCached) noexcept
      : Instance(Instance) {
    if constexpr (!IsSOOCandidate<EF>) {
      this->IsCached = IsCached;
    }
  }
  const void *Instance{};
  [[no_unique_address]] std::conditional_t<IsSOOCandidate<EF>, EmptyType, bool>
      IsCached{};
};

/// Ref-counted and type-erased edge function with small-object optimization.
/// Supports caching.
template <typename L>
// -- combined copy and move assignment
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class [[clang::trivial_abi]] EdgeFunction final : EdgeFunctionBase {
public:
  using l_t = L;

  // --- Constructors

  /// Default-initializes the edge-function with nullptr
  EdgeFunction() noexcept = default;
  /// Default-initializes the edge-function with nullptr
  EdgeFunction(std::nullptr_t) noexcept : EdgeFunction() {}
  /// Copy constructor. Increments the ref-count, if not small-object-optimized.
  EdgeFunction(const EdgeFunction &Other) noexcept
      : EdgeFunction(Other.EF, Other.VTAndHeapAlloc) {}
  /// Move constructor. Does not increment the ref-count, but instead leaves the
  /// moved-from edge function in the nullptr state.
  EdgeFunction(EdgeFunction &&Other) noexcept
      : EF(std::exchange(Other.EF, nullptr)),
        VTAndHeapAlloc(
            std::exchange(Other.VTAndHeapAlloc, decltype(VTAndHeapAlloc){})) {}

  /// Standard swap; does not affect ref-counts
  void swap(EdgeFunction &Other) noexcept {
    std::swap(EF, Other.EF);
    std::swap(VTAndHeapAlloc, Other.VTAndHeapAlloc);
  }
  /// Standard swap; does not affect ref-counts
  friend void swap(EdgeFunction &LHS, EdgeFunction &RHS) noexcept {
    LHS.swap(RHS);
  }

  /// Combined copy- and move assignment. If the assigned-to edge function is
  /// not null, invokes the destructor on it, before overwriting its content.
  EdgeFunction &operator=(EdgeFunction Other) noexcept {
    std::destroy_at(this);
    return *new (this) EdgeFunction(std::move(Other)); // NOLINT
  }
  /// Null-assignment operator. Decrements the ref-count if not
  /// small-object-optimized or already null. Leaves the assigned-to edge
  /// function in the nullptr state.
  EdgeFunction &operator=(std::nullptr_t) noexcept {
    std::destroy_at(this);
    return *new (this) EdgeFunction(); // NOLINT
  }

  /// Destructor. Decrements the ref-count if not small-object-optimized and
  /// destroyes the held edge function once the ref-count reaches 0.
  ~EdgeFunction() noexcept {
    AllocationPolicy Policy = VTAndHeapAlloc.getInt();
    if (Policy != AllocationPolicy::SmallObjectOptimized) {
      assert(VTAndHeapAlloc.getPointer() != nullptr && "Heap-alloc'd nullptr?");
      // Note: Memory-order taken from llvm::ThreadSafeRefCountedBase
      if (static_cast<const RefCountedBase *>(EF)->Rc.fetch_sub(
              1, std::memory_order_acq_rel) == 1) {
        VTAndHeapAlloc.getPointer()->destroy(EF, Policy);
      }
    }
  }

  /// Implicit-conversion constructor from EdgeFunctionRef. Increments the
  /// ref-count if not small-object optimized
  template <typename ConcreteEF, typename = std::enable_if_t<!std::is_same_v<
                                     EdgeFunction, std::decay_t<ConcreteEF>>>>
  EdgeFunction(EdgeFunctionRef<ConcreteEF> CEF) noexcept
      : EdgeFunction(CEF.Instance,
                     {&VTableFor<ConcreteEF>, [CEF] {
                        if constexpr (IsSOOCandidate<ConcreteEF>) {
                          (void)CEF;
                          return AllocationPolicy::SmallObjectOptimized;
                        } else {
                          return CEF.IsCached
                                     ? AllocationPolicy::CustomHeapAllocated
                                     : AllocationPolicy::DefaultHeapAllocated;
                        }
                      }()}) {}

  /// Conversion-constructor from any edge function (that satisfies the
  /// IsEdgeFunction trait). Stores a type-erased copy of CEF and allocates
  /// space for it on the heap if small-object-optimization cannot be applied.
  template <typename ConcreteEF,
            typename = std::enable_if_t<
                !std::is_same_v<EdgeFunction, std::decay_t<ConcreteEF>> &&
                IsEdgeFunction<ConcreteEF>>>
  EdgeFunction(ConcreteEF &&CEF) noexcept(
      IsSOOCandidate<std::decay_t<ConcreteEF>>)
      : EdgeFunction(std::in_place_type<std::decay_t<ConcreteEF>>,
                     std::forward<ConcreteEF>(CEF)) {}

  /// Emplacement-constructor for any edge function. Constructs a new object of
  /// type ConcreteEF with the given constructor arguments and allocates space
  /// for it on the heap if small-object-optimization cannot be applied.
  /// No extra copy- or move construction/assignment is performed. Use this ctor
  /// if even moving is expensive.
  template <typename ConcreteEF, typename... ArgTys>
  explicit EdgeFunction(
      std::in_place_type_t<ConcreteEF> /*unused*/,
      ArgTys &&...Args) noexcept(IsSOOCandidate<std::decay_t<ConcreteEF>> &&
                                     std::is_nothrow_constructible_v<ConcreteEF,
                                                                     ArgTys...>)
      : EdgeFunction(
            [](auto &&...Args) {
              if constexpr (IsSOOCandidate<std::decay_t<ConcreteEF>>) {
                void *Ret = nullptr;
                new (&Ret) ConcreteEF(std::forward<ArgTys>(Args)...);
                return Ret;
              } else {
                return new RefCounted<ConcreteEF>{
                    {}, {std::forward<ArgTys>(Args)...}};
              }
            }(std::forward<ArgTys>(Args)...),
            {&VTableFor<ConcreteEF>, DefaultAllocPolicy<ConcreteEF>}) {
    static_assert(std::is_same_v<l_t, typename ConcreteEF::l_t>,
                  "Cannot construct EdgeFunction with incompatible "
                  "lattice domain");
  }

  /// Conversion-constructor for any edge function with enabled caching.  Stores
  /// a type-erased copy of EF.EF and allocates space for it on the heap if
  /// small-object-optimization cannot be applied. If a heap allocation is
  /// requires, first consults the EF.Cache to check whether an equivalent edge
  /// function is already allocated. If so, takes the one from the cache and
  /// increases its ref-count. Otherwise, performs a fresh allocation and
  /// inserts the edge function into EF.Cache.
  /// When all references to this edge function are out-of-scope, the destructor
  /// automatically removes the edge function from EF.Cache. Hence, make sure
  /// that EF.Cache lives at least as long as the last edge function cached in
  /// it.
  template <typename ConcreteEF, typename = std::enable_if_t<
                                     IsEdgeFunction<ConcreteEF> &&
                                     std::is_move_constructible_v<ConcreteEF>>>
  EdgeFunction(CachedEdgeFunction<ConcreteEF> EF)
      : EdgeFunction(
            [&EF] {
              assert(EF.Cache != nullptr);
              if constexpr (IsSOOCandidate<std::decay_t<ConcreteEF>>) {
                void *Ret;
                new (&Ret) ConcreteEF(std::move(EF.EF));
                return Ret;
              } else {
                if (const auto *Mem = EF.Cache->lookup(EF.EF)) {
                  return static_cast<const RefCounted<ConcreteEF> *>(Mem);
                }

                auto Ret = new CachedRefCounted<ConcreteEF>{
                    {{}, {std::move(EF.EF)}}, EF.Cache};
                EF.Cache->insert(&Ret->Value, Ret);
                return static_cast<const RefCounted<ConcreteEF> *>(Ret);
              }
            }(),
            {&VTableFor<ConcreteEF>, CustomAllocPolicy<ConcreteEF>}) {}

  // ---  API functions

  ///
  /// This function describes the concrete value computation for its respective
  /// exploded supergraph edge. The function(s) will be evaluated once the
  /// exploded supergraph has been constructed and the concrete values of the
  /// various value computation problems along the supergraph edges are
  /// evaluated.
  ///
  /// Please also refer to the various edge function factories of the
  /// EdgeFunctions interface: EdgeFunctions::get*EdgeFunction() for more
  /// details.
  ///
  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> Source) const {
    assert(!!*this && "computeTarget() called on nullptr!");
    return VTAndHeapAlloc.getPointer()->computeTarget(EF, Source);
  }

  ///
  /// This function composes the two edge functions this and SecondEF. This
  /// function is used to extend an edge function in order to construct
  /// so-called jump functions that describe the effects of everlonger sequences
  /// of code.
  ///
  /// Calls the static function EF::compose(*this, SecondEF) for your concrete
  /// edge function EF.
  ///
  /// For semantic correctness, please make sure that for all inputs x in l_t,
  /// it holds: this->composeWith(SecondEF).computeTarget(x) ==
  /// SecondEF.computeTarget(this->computeTarget(x)).
  [[nodiscard]] EdgeFunction composeWith(const EdgeFunction &SecondEF) const {
    return compose(*this, SecondEF);
  }

  ///
  /// This function composes the two edge functions this and SecondEF. This
  /// function is used to extend an edge function in order to construct
  /// so-called jump functions that describe the effects of everlonger sequences
  /// of code.
  ///
  /// For semantic correctness, please make sure that for all inputs x in l_t,
  /// it holds: compose(FirstEF, SecondEF).computeTarget(x) ==
  /// SecondEF.computeTarget(FirstEF.computeTarget(x)).
  [[nodiscard]] static EdgeFunction compose(const EdgeFunction &FirstEF,
                                            const EdgeFunction &SecondEF) {
    assert(!!FirstEF && "compose() called on LHS nullptr!");
    assert(!!SecondEF && "compose() called on RHS nullptr!");
    return FirstEF.VTAndHeapAlloc.getPointer()->compose(
        FirstEF.EF, SecondEF, FirstEF.VTAndHeapAlloc.getInt());
  }

  ///
  /// This function describes the join of the two edge functions this and
  /// OtherEF. The function is called whenever two edge functions need to
  /// be joined, for instance, when two branches lead to a common successor
  /// instruction.
  ///
  /// Calls the static function EF::join(*this, OtherEF) for your concrete
  /// edge function EF.
  ///
  /// For semantic correctness, please make sure that for all inputs x in l_t,
  /// it holds (with join = JoinLatticeTraits<l_t>::join):
  /// join(this->joinWith(OtherEF).computeTarget(x)),
  ///      join(this->computeTarget(x), OtherEF.computeTarget(x))) ==
  /// this->joinWith(OtherEF).computeTarget(x)
  /// i.e. that joining on edge functions only goes up the lattice that is
  /// connected with the value-lattice on l_t
  [[nodiscard]] EdgeFunction joinWith(const EdgeFunction &OtherEF) const {
    return join(*this, OtherEF);
  }

  ///
  /// This function describes the join of the two edge functions this and
  /// OtherEF. The function is called whenever two edge functions need to
  /// be joined, for instance, when two branches lead to a common successor
  /// instruction.
  ///
  /// For semantic correctness, please make sure that for all inputs x in l_t,
  /// it holds (with joinl = JoinLatticeTraits<l_t>::join):
  /// joinl(join(FirstEF, SecondEF).computeTarget(x)),
  ///       joinl(FirstEF.computeTarget(x), SecondEF.computeTarget(x))) ==
  /// this->joinWith(OtherEF).computeTarget(x)
  /// i.e. that joining on edge functions only goes up the lattice that is
  /// connected with the value-lattice on l_t
  [[nodiscard]] static EdgeFunction join(const EdgeFunction &FirstEF,
                                         const EdgeFunction &SecondEF) {
    assert(!!FirstEF && "join() called on LHS nullptr!");
    assert(!!SecondEF && "join() called on RHS nullptr!");
    return FirstEF.VTAndHeapAlloc.getPointer()->join(
        FirstEF.EF, SecondEF, FirstEF.VTAndHeapAlloc.getInt());
  }

  /// Checks for equality of two edge functions. Equality requires exact
  /// type-equality and value-equality based on operator== of the concrete edge
  /// functions that are compared.
  ///
  /// If the concrete edge function has no nonstatic data members, the equality
  /// is defaulted to only compare the types. However, by explicitly defining
  /// operator==, this behavior can be overridden.
  [[nodiscard]] friend bool operator==(const EdgeFunction &LHS,
                                       const EdgeFunction &RHS) noexcept {
    if (LHS.VTAndHeapAlloc.getPointer() != RHS.VTAndHeapAlloc.getPointer()) {
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

  /// Printing function. Based on llvm::raw_ostream
  /// &operator<<(llvm::raw_ostream &OS, const ConcreteEF &EF) for the concrete
  /// type ConcreteEF of EF.
  ///
  /// If the ConcreteEF does not define a fitting printing function, defaults to
  /// printing the concrete type ConcreteEF.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeFunction &EF) {
    if (!EF) {
      return OS << "<null-EF>";
    }

    EF.VTAndHeapAlloc.getPointer()->print(EF.EF, OS);
    return OS;
  }

  /// Printing function. Based on llvm::raw_ostream
  /// &operator<<(llvm::raw_ostream &OS, const EdgeFunction &EF).
  ///
  /// Useful for unittests (gtest works with std::ostream instead of
  /// llvm::raw_ostream).
  friend std::ostream &operator<<(std::ostream &OS, const EdgeFunction &EF) {
    llvm::raw_os_ostream ROS(OS);
    ROS << EF;
    return OS;
  }

  /// Stringify function. Based on llvm::raw_ostream
  /// &operator<<(llvm::raw_ostream &OS, const EdgeFunction &EF).
  ///
  /// Useful for ADL calls to to_string (overloads with all the to_string
  /// functions of the STL). Do not rename!
  [[nodiscard]] friend std::string to_string(const EdgeFunction &EF) {
    std::string Ret;
    llvm::raw_string_ostream ROS(Ret);
    ROS << EF;
    return Ret;
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

  /// True, if the concrete edge function defines itself as constant, i.e.
  /// for all x,y in l_t it holds: computeTarget(x) == computeTarget(y).
  ///
  /// Allows for better optimizations in compose and join and should be
  /// provided, whehever this knowledge is available.
  [[nodiscard]] bool isConstant() const noexcept {
    assert(!!*this && "isConstant() called on nullptr!");
    return VTAndHeapAlloc.getPointer()->isConstant(EF);
  }

  /// Performs a null-check. True, iff thie edge function is not null.
  [[nodiscard]] explicit operator bool() const noexcept {
    return VTAndHeapAlloc.getOpaqueValue();
  }

  /// Performs a runtime-typecheck. True, if the concrete type of the held edge
  /// function *exactly* equals ConcreteEF.
  ///
  /// CAUTION: This model of isa-relation does not care about inheritance. Edge
  /// functions of a base class BaseEF and a derived class DerivedEF are
  /// considered unrelated!
  template <typename ConcreteEF> [[nodiscard]] bool isa() const noexcept {
    if constexpr (IsEdgeFunction<ConcreteEF> &&
                  std::is_same_v<l_t, typename ConcreteEF::l_t>) {
      return VTAndHeapAlloc.getPointer() == &VTableFor<ConcreteEF>;
    } else {
      return false;
    }
  }

  /// Performs an *unchecked* typecast to ConcreteEF. In debug builds, asserts
  /// isa<ConcreteEF>.
  ///
  /// Compatible with the llvm::isa API.
  ///
  /// Use with caution!
  template <typename ConcreteEF>
  [[nodiscard]] const ConcreteEF *cast() const noexcept {
    assert(this->template isa<ConcreteEF>() && "Cast on incompatible type!");
    return getPtr<ConcreteEF>(EF);
  }

  /// Performs an *checked* typecast to ConcreteEF. If this edge function is not
  /// *exactly* of type ConcreteEF, returns nullptr.
  ///
  /// Compatible with the llvm::dyn_cast API.
  template <typename ConcreteEF>
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] const ConcreteEF *dyn_cast() const noexcept {
    return this->template isa<ConcreteEF>() ? getPtr<ConcreteEF>(EF) : nullptr;
  }

  // -- misc

  /// True, iff this edge function is not small-object-optimized and thus its
  /// lifetime is managed by ref-counting.
  ///
  /// False for null-EF.
  [[nodiscard]] bool isRefCounted() const noexcept {
    return VTAndHeapAlloc.getInt() != AllocationPolicy::SmallObjectOptimized;
  }

  /// True, iff this edge function is cached in a EdgeFunctionSingletonCache.
  ///
  /// False for small-object-optimized- and null-EF.
  [[nodiscard]] bool isCached() const noexcept {
    return VTAndHeapAlloc.getInt() == AllocationPolicy::CustomHeapAllocated;
  }

  /// Gets an opaque identifier for this edge function. Only meant for
  /// comparisons of object-identity. Do not dereference!
  [[nodiscard]] const void *getOpaqueValue() const noexcept { return EF; }

  /// Gets the cache where this edge function is being cached in. If this edge
  /// function is not cached (i.e., isCached() returns false), returns nullptr.
  /// Assumes that the held edge function is *exactly* of type ConcreteEF. In
  /// debug builds, asserts isa<ConcreteEF>.
  ///
  /// Use with caution!
  template <typename ConcreteEF>
  [[nodiscard]] EdgeFunctionSingletonCache<ConcreteEF> *
  getCacheOrNull() const noexcept {
    assert(isa<ConcreteEF>());
    if (IsSOOCandidate<ConcreteEF> ||
        VTAndHeapAlloc.getInt() == AllocationPolicy::DefaultHeapAllocated) {
      return nullptr;
    }
    return static_cast<const CachedRefCounted<ConcreteEF> *>(EF)->Cache;
  }

private:
  struct VTable {
    // NOLINTBEGIN(readability-identifier-naming)
    l_t (*computeTarget)(const void *, ByConstRef<l_t>);
    EdgeFunction (*compose)(const void *, const EdgeFunction &,
                            AllocationPolicy);
    EdgeFunction (*join)(const void *, const EdgeFunction &, AllocationPolicy);
    bool (*equals)(const void *, const void *) noexcept;
    void (*print)(const void *, llvm::raw_ostream &);
    bool (*isConstant)(const void *) noexcept;
    void (*destroy)(const void *, AllocationPolicy) noexcept;
    // NOLINTEND(readability-identifier-naming)
  };

  template <typename ConcreteEF>
  static constexpr VTable VTableFor = {
      [](const void *EF, ByConstRef<l_t> Source) {
        return getPtr<ConcreteEF>(EF)->computeTarget(Source);
      },
      [](const void *EF, const EdgeFunction &SecondEF,
         AllocationPolicy Policy) {
        return ConcreteEF::compose(
            EdgeFunctionRef<ConcreteEF>(
                EF, Policy == AllocationPolicy::CustomHeapAllocated),
            SecondEF);
      },
      [](const void *EF, const EdgeFunction &OtherEF, AllocationPolicy Policy) {
        return ConcreteEF::join(
            EdgeFunctionRef<ConcreteEF>(
                EF, Policy == AllocationPolicy::CustomHeapAllocated),
            OtherEF);
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
      [](const void *EF, AllocationPolicy Policy) noexcept {
        if constexpr (!IsSOOCandidate<ConcreteEF>) {
          if (Policy != AllocationPolicy::CustomHeapAllocated) {
            assert(Policy == AllocationPolicy::DefaultHeapAllocated);
            delete static_cast<const RefCounted<ConcreteEF> *>(EF);
          } else {
            auto CEF = static_cast<const CachedRefCounted<ConcreteEF> *>(EF);
            CEF->Cache->erase(CEF->Value);
            delete CEF;
          }
        }
      },
  };

  // Utility ctor for (copy) construction. Increments the ref-count if
  // necessary
  explicit EdgeFunction(
      const void *EF, llvm::PointerIntPair<const VTable *, 2, AllocationPolicy>
                          VTAndHeapAlloc) noexcept
      : EF(EF), VTAndHeapAlloc(VTAndHeapAlloc) {
    if (VTAndHeapAlloc.getInt() != AllocationPolicy::SmallObjectOptimized) {
      // Note: Memory-order taken from llvm::ThreadSafeRefCountedBase
      static_cast<const RefCountedBase *>(EF)->Rc.fetch_add(
          1, std::memory_order_relaxed);
    }
  }

  // -- data members

  const void *EF{};
  llvm::PointerIntPair<const VTable *, 2, AllocationPolicy> VTAndHeapAlloc{};
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

#endif // PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTION_H
