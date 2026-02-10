/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_ALIASINFOBASE_H
#define PHASAR_POINTER_ALIASINFOBASE_H

#include "phasar/Pointer/AliasInfoTraits.h"

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json_fwd.hpp"

#include <concepts>

namespace llvm {
class Function;
class Value;
} // namespace llvm

namespace psr {

enum class AliasAnalysisType;
enum class AnalysisProperties;
enum class AliasResult;

class AliasInfoBaseUtils {
public:
  static const llvm::Function *retrieveFunction(const llvm::Value *V);
};

template <typename T>
concept IsAliasInfo =
    requires(const T &CVal, T &MutVal, typename AliasInfoTraits<T>::v_t Ptr,
             typename AliasInfoTraits<T>::n_t Inst) {
      CVal.print(llvm::outs());
      CVal.printAsJson(llvm::outs());
      MutVal.mergeWith(MutVal);
      MutVal.introduceAlias(Ptr, Ptr, Inst, AliasResult{});

      { CVal.isInterProcedural() } -> std::convertible_to<bool>;
      { CVal.getAliasAnalysisType() } -> std::same_as<AliasAnalysisType>;
      { MutVal.alias(Ptr, Ptr, Inst) } -> std::same_as<AliasResult>;
      {
        MutVal.getAliasSet(Ptr, Inst)
      } -> std::same_as<typename AliasInfoTraits<T>::AliasSetPtrTy>;
      {
        MutVal.getReachableAllocationSites(Ptr, bool{}, Inst)
      } -> std::same_as<typename AliasInfoTraits<T>::AllocationSiteSetPtrTy>;
      {
        MutVal.isInReachableAllocationSites(Ptr, Ptr, bool{}, Inst)
      } -> std::convertible_to<bool>;
      { CVal.getAnalysisProperties() } -> std::same_as<AnalysisProperties>;
      { CVal.isContextSensitive() } -> std::convertible_to<bool>;
      { CVal.isFieldSensitive() } -> std::convertible_to<bool>;
      { CVal.isFlowSensitive() } -> std::convertible_to<bool>;
    };

} // namespace psr

#endif // PHASAR_POINTER_ALIASINFOBASE_H
