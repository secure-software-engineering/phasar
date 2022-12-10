/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_ALIASINFO_H_
#define PHASAR_PHASARLLVM_POINTER_ALIASINFO_H_

#include "phasar/PhasarLLVM/Pointer/AliasAnalysisType.h"
#include "phasar/PhasarLLVM/Pointer/AliasInfoBase.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TypeName.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace llvm {
class Value;
class Instruction;
} // namespace llvm

namespace psr {

template <typename V, typename N> class AliasInfoRef;

template <typename V, typename N> class AliasInfo;
template <typename V, typename N>
struct AliasInfoTraits<AliasInfoRef<V, N>> : DefaultAATraits<V, N> {};
template <typename V, typename N>
struct AliasInfoTraits<AliasInfo<V, N>> : DefaultAATraits<V, N> {};

namespace detail {
template <typename ConcreteAA, typename = void>
struct CanMergeWithAARef : std::false_type {};
template <typename ConcreteAA>
struct CanMergeWithAARef<
    ConcreteAA, std::void_t<decltype(std::declval<ConcreteAA &>().mergeWith(
                    std::declval<AliasInfoRef<typename ConcreteAA::v_t,
                                              typename ConcreteAA::n_t>>()))>>
    : std::true_type {};
} // namespace detail

/// A type-erased reference to any object implementing the AliasInfoBase
/// interface. Use this, if your analysis is not tied to a specific alias info
/// implementation.
///
/// This is a *non-owning* reference similar to std::string_view and
/// llvm::ArrayRef. Pass this type by value.
///
/// Example:
/// \code
/// LLVMAliasSet ASet(...);
/// AliasInfoRef AA = &ASet;
/// \endcode
///
template <typename V, typename N>
class AliasInfoRef : public AliasInfoBase<AliasInfoRef<V, N>> {
  friend AliasInfoBase<AliasInfoRef<V, N>>;
  friend class AliasInfo<V, N>;
  using base_t = AliasInfoBase<AliasInfoRef<V, N>>;

public:
  using typename base_t::AliasSetPtrTy;
  using typename base_t::AliasSetTy;
  using typename base_t::AllocationSiteSetPtrTy;
  using typename base_t::n_t;
  using typename base_t::v_t;

  AliasInfoRef() noexcept = default;
  AliasInfoRef(std::nullptr_t) : AliasInfoRef() {}
  template <typename ConcreteAA,
            typename = std::enable_if_t<
                !std::is_base_of_v<AliasInfoRef, ConcreteAA> &&
                std::is_same_v<v_t, typename ConcreteAA::v_t> &&
                std::is_same_v<n_t, typename ConcreteAA::n_t>>>
  AliasInfoRef(ConcreteAA *AA) : AA(AA), VT(&VTableFor<ConcreteAA>) {
    if constexpr (!std::is_empty_v<ConcreteAA>) {
      assert(AA != nullptr);
    }
  }

  AliasInfoRef(const AliasInfoRef &) noexcept = default;
  AliasInfoRef &operator=(const AliasInfoRef &) noexcept = default;
  ~AliasInfoRef() noexcept = default;

  explicit operator bool() const noexcept { return VT != nullptr; }

private:
  struct VTable {
    bool (*IsInterProcedural)(const void *) noexcept;
    AliasAnalysisType (*GetAliasAnalysisType)(const void *) noexcept;
    AliasResult (*Alias)(void *, ByConstRef<v_t>, ByConstRef<v_t>,
                         ByConstRef<n_t>);
    AliasSetPtrTy (*GetAliasSet)(void *, ByConstRef<v_t>, ByConstRef<n_t>);
    AllocationSiteSetPtrTy (*GetReachableAllocationSites)(void *,
                                                          ByConstRef<v_t>, bool,
                                                          ByConstRef<n_t>);
    bool (*IsInReachableAllocationSites)(void *, ByConstRef<v_t>,
                                         ByConstRef<v_t>, bool,
                                         ByConstRef<n_t>);
    void (*Print)(const void *, llvm::raw_ostream &);
    nlohmann::json (*GetAsJson)(const void *);
    void (*PrintAsJson)(const void *, llvm::raw_ostream &);
    void (*MergeWith)(void *, AliasInfoRef<v_t, n_t>);
    void (*IntroduceAlias)(void *, ByConstRef<v_t>, ByConstRef<v_t>,
                           ByConstRef<n_t>, AliasResult);
    AnalysisProperties (*GetAnalysisProperties)(const void *) noexcept;
    void (*Destroy)(void *);
  };

  template <typename ConcreteAA>
  static constexpr VTable VTableFor = {
      [](const void *AA) noexcept {
        return static_cast<const ConcreteAA *>(AA)->isInterProcedural();
      },
      [](const void *AA) noexcept {
        return static_cast<const ConcreteAA *>(AA)->getAliasAnalysisType();
      },
      [](void *AA, ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
         ByConstRef<n_t> AtInstruction) {
        return static_cast<ConcreteAA *>(AA)->alias(Pointer1, Pointer2,
                                                    AtInstruction);
      },
      [](void *AA, ByConstRef<v_t> Pointer, ByConstRef<n_t> AtInstruction) {
        return static_cast<ConcreteAA *>(AA)->getAliasSet(Pointer,
                                                          AtInstruction);
      },
      [](void *AA, ByConstRef<v_t> Pointer, bool IntraProcOnly,
         ByConstRef<n_t> AtInstruction) {
        return static_cast<ConcreteAA *>(AA)->getReachableAllocationSites(
            Pointer, IntraProcOnly, AtInstruction);
      },
      [](void *AA, ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
         bool IntraProcOnly, ByConstRef<n_t> AtInstruction) {
        return static_cast<ConcreteAA *>(AA)->isInReachableAllocationSites(
            Pointer1, Pointer2, IntraProcOnly, AtInstruction);
      },
      [](const void *AA, llvm::raw_ostream &OS) {
        static_cast<const ConcreteAA *>(AA)->print(OS);
      },
      [](const void *AA) {
        return static_cast<const ConcreteAA *>(AA)->getAsJson();
      },
      [](const void *AA, llvm::raw_ostream &OS) {
        static_cast<const ConcreteAA *>(AA)->printAsJson(OS);
      },
      [](void *AA, AliasInfoRef<v_t, n_t> Other) {
        if constexpr (detail::CanMergeWithAARef<ConcreteAA>::value) {
          static_cast<ConcreteAA *>(AA)->mergeWith(Other);
        } else {
          llvm::report_fatal_error("Cannot merge alias-info of type " +
                                   llvm::getTypeName<ConcreteAA>() +
                                   " with type-erased alias-info");
        }
      },
      [](void *AA, ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
         ByConstRef<n_t> AtInstruction, AliasResult Kind) {
        static_cast<ConcreteAA *>(AA)->introduceAlias(Pointer1, Pointer2,
                                                      AtInstruction, Kind);
      },
      [](const void *AA) noexcept {
        return static_cast<const ConcreteAA *>(AA)->getAnalysisProperties();
      },
      [](void *AA) { delete static_cast<ConcreteAA *>(AA); },
  };

  // -- Impl for AliasInfoBase:
  [[nodiscard]] bool isInterProceduralImpl() const noexcept {
    assert(VT != nullptr);
    return VT->IsInterProcedural(AA);
  }
  [[nodiscard]] AliasAnalysisType getAliasAnalysisTypeImpl() const noexcept {
    assert(VT != nullptr);
    return VT->GetAliasAnalysisType(AA);
  }

  [[nodiscard]] AliasResult
  aliasImpl(ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
            ByConstRef<n_t> AtInstruction = {}) const {
    assert(VT != nullptr);
    return VT->Alias(AA, Pointer1, Pointer2, AtInstruction);
  }

  [[nodiscard]] AliasSetPtrTy
  getAliasSetImpl(ByConstRef<v_t> Pointer,
                  ByConstRef<n_t> AtInstruction = {}) const {
    assert(VT != nullptr);
    return VT->GetAliasSet(AA, Pointer, AtInstruction);
  }

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSitesImpl(ByConstRef<v_t> Pointer,
                                  bool IntraProcOnly = false,
                                  ByConstRef<n_t> AtInstruction = {}) const {
    assert(VT != nullptr);
    return VT->GetReachableAllocationSites(AA, Pointer, IntraProcOnly,
                                           AtInstruction);
  }

  // Checks if Pointer2 is a reachable allocation in the alias set of
  // Pointer1.
  [[nodiscard]] bool isInReachableAllocationSitesImpl(
      ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
      bool IntraProcOnly = false, ByConstRef<n_t> AtInstruction = {}) const {
    assert(VT != nullptr);
    return VT->IsInReachableAllocationSites(AA, Pointer1, Pointer2,
                                            IntraProcOnly, AtInstruction);
  }

  void printImpl(llvm::raw_ostream &OS = llvm::outs()) const {
    assert(VT != nullptr);
    VT->Print(AA, OS);
  }

  [[nodiscard]] nlohmann::json getAsJsonImpl() const {
    assert(VT != nullptr);
    return VT->GetAsJson(AA);
  }

  void printAsJsonImpl(llvm::raw_ostream &OS) const {
    assert(VT != nullptr);
    VT->PrintAsJson(AA, OS);
  }

  template <typename AI,
            typename = std::enable_if_t<std::is_same_v<typename AI::n_t, n_t> &&
                                        std::is_same_v<typename AI::v_t, v_t>>>
  void mergeWithImpl(const AliasInfoBase<AI> &Other) {
    assert(VT != nullptr);
    VT->MergeWith(AA, &Other);
  }

  void introduceAliasImpl(ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
                          ByConstRef<n_t> AtInstruction = {},
                          AliasResult Kind = AliasResult::MustAlias) {
    assert(VT != nullptr);
    VT->IntroduceAlias(AA, Pointer1, Pointer2, AtInstruction, Kind);
  }

  [[nodiscard]] AnalysisProperties getAnalysisPropertiesImpl() const noexcept {
    assert(VT != nullptr);
    return VT->GetAnalysisProperties(AA);
  }

  // --

  void *AA{};
  const VTable *VT{};
};

/// Similar to AliasInfoRef, but owns the held reference. Us this, if you need
/// to decide dynamically, which alias info implementation to use.
///
/// Implicitly convertible to AliasInfoRef.
///
/// Example:
/// \code
/// AliasInfo AA = std::make_unique<LLVMAliasSet>(...);
/// \endcode
///
template <typename V, typename N>
class AliasInfo final : public AliasInfoRef<V, N> {
  using base_t = AliasInfoRef<V, N>;

public:
  using typename base_t::AliasSetPtrTy;
  using typename base_t::AliasSetTy;
  using typename base_t::AllocationSiteSetPtrTy;
  using typename base_t::n_t;
  using typename base_t::v_t;

  AliasInfo() noexcept = default;
  AliasInfo(std::nullptr_t) noexcept {};
  AliasInfo(const AliasInfo &) = delete;
  AliasInfo &operator=(const AliasInfo &) = delete;
  AliasInfo(AliasInfo &&Other) noexcept { swap(Other); }
  AliasInfo &operator=(AliasInfo &&Other) noexcept {
    auto Cpy{std::move(Other)};
    swap(Cpy);
    return *this;
  }

  void swap(AliasInfo &Other) noexcept {
    std::swap(this->AA, Other.AA);
    std::swap(this->VT, Other.VT);
  }
  friend void swap(AliasInfo &LHS, AliasInfo &RHS) noexcept { LHS.swap(RHS); }

  template <typename ConcretePTA, typename... ArgTys>
  explicit AliasInfo(std::in_place_type_t<ConcretePTA> /*unused*/,
                     ArgTys &&...Args)
      : base_t(new ConcretePTA(std::forward<ArgTys>(Args)...)) {}

  template <typename ConcretePTA>
  AliasInfo(std::unique_ptr<ConcretePTA> AA) : base_t(AA.release()) {}

  ~AliasInfo() noexcept {
    if (*this) {
      this->VT->Destroy(this->AA);
      this->VT = nullptr;
      this->AA = nullptr;
    }
  }

  [[nodiscard]] base_t asRef() noexcept { return *this; }
  [[nodiscard]] AliasInfoRef<V, N> asRef() const noexcept { return *this; }

  /// For better interoperability with unique_ptr
  [[nodiscard]] base_t get() noexcept { return asRef(); }
  [[nodiscard]] AliasInfoRef<V, N> get() const noexcept { return asRef(); }
};

extern template class AliasInfoBase<
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *>>;
extern template class AliasInfoRef<const llvm::Value *,
                                   const llvm::Instruction *>;
extern template class AliasInfo<const llvm::Value *, const llvm::Instruction *>;
} // namespace psr

#endif
