/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_ALIASINFOBASE_H
#define PHASAR_PHASARLLVM_POINTER_ALIASINFOBASE_H

#include "phasar/PhasarLLVM/Pointer/AliasInfoTraits.h"
#include "phasar/PhasarLLVM/Pointer/AliasResult.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <memory>
#include <type_traits>

namespace llvm {
class Function;
class Value;
} // namespace llvm

namespace psr {

enum class AliasAnalysisType;

class AliasInfoBaseUtils {
public:
  static const llvm::Function *retrieveFunction(const llvm::Value *V);
};

/// Base class of all alias analysis implementations.
/// Don't use this class directly. For a type-erased variant, use AliasInfoRef
/// or AliasInfo.
template <typename Derived> class AliasInfoBase : public AliasInfoBaseUtils {
public:
  using n_t = typename AliasInfoTraits<Derived>::n_t;
  using v_t = typename AliasInfoTraits<Derived>::v_t;
  using AliasSetTy = typename AliasInfoTraits<Derived>::AliasSetTy;
  using AliasSetPtrTy = typename AliasInfoTraits<Derived>::AliasSetPtrTy;
  using AllocationSiteSetPtrTy =
      typename AliasInfoTraits<Derived>::AllocationSiteSetPtrTy;

  AliasInfoBase() noexcept {
    static_assert(std::is_base_of_v<AliasInfoBase, Derived>,
                  "Invalid CRTP instantiation: Derived must inherit from "
                  "AliasInfoBase<Derived>!");
  }

  [[nodiscard]] bool isInterProcedural() const noexcept {
    return self().isInterProceduralImpl();
  }
  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return self().getAliasAnalysisTypeImpl();
  }

  [[nodiscard]] AliasResult alias(ByConstRef<v_t> Pointer1,
                                  ByConstRef<v_t> Pointer2,
                                  ByConstRef<n_t> AtInstruction = {}) {
    return self().aliasImpl(Pointer1, Pointer2, AtInstruction);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(ByConstRef<v_t> Pointer,
                                          ByConstRef<n_t> AtInstruction = {}) {
    return self().getAliasSetImpl(Pointer, AtInstruction);
  }

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSites(ByConstRef<v_t> Pointer,
                              bool IntraProcOnly = false,
                              ByConstRef<n_t> AtInstruction = {}) {
    return self().getReachableAllocationSitesImpl(Pointer, IntraProcOnly,
                                                  AtInstruction);
  }

  // Checks if Pointer2 is a reachable allocation in the alias set of
  // Pointer1.
  [[nodiscard]] bool isInReachableAllocationSites(
      ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
      bool IntraProcOnly = false, ByConstRef<n_t> AtInstruction = {}) {
    return self().isInReachableAllocationSitesImpl(
        Pointer1, Pointer2, IntraProcOnly, AtInstruction);
  }

  void print(llvm::raw_ostream &OS = llvm::outs()) const {
    self().printImpl(OS);
  }

  [[nodiscard]] nlohmann::json getAsJson() const {
    return self().getAsJsonImpl();
  }

  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const {
    self().printAsJsonImpl(OS);
  }

  template <typename AI, typename DD = Derived,
            typename = std::enable_if_t<std::is_same_v<typename AI::n_t, n_t> &&
                                        std::is_same_v<typename AI::v_t, v_t>>,
            typename = decltype(std::declval<DD &>().mergeWithImpl(
                std::declval<const AI &>()))>
  void mergeWith(const AliasInfoBase<AI> &Other) {
    self().mergeWithImpl(Other);
  }

  void introduceAlias(ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
                      ByConstRef<n_t> AtInstruction = {},
                      AliasResult Kind = AliasResult::MustAlias) {
    self().introduceAliasImpl(Pointer1, Pointer2, AtInstruction, Kind);
  }

  [[nodiscard]] AnalysisProperties getAnalysisProperties() const noexcept {
    return self().getAnalysisPropertiesImpl();
  }

  [[nodiscard]] bool isFieldSensitive() const noexcept {
    return hasFlag(getAnalysisProperties(), AnalysisProperties::FieldSensitive);
  }

  [[nodiscard]] bool isContextSensitive() const noexcept {
    return hasFlag(getAnalysisProperties(),
                   AnalysisProperties::ContextSensitive);
  }

  [[nodiscard]] bool isFlowSensitive() const noexcept {
    return hasFlag(getAnalysisProperties(), AnalysisProperties::FlowSensitive);
  }

private:
  [[nodiscard]] Derived &self() noexcept {
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

template <typename AI,
          typename = std::enable_if_t<std::is_base_of_v<AliasInfoBase<AI>, AI>>>
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const AI &Info) {
  Info.print(OS);
  return OS;
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_ALIASINFOBASE_H
