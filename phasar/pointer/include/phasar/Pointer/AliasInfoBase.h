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

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <optional>
#include <tuple>
#include <type_traits>

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

namespace detail {

template <typename T>
auto testAliasInfo(
    T &AI, const T &CAI,
    const std::optional<typename AliasInfoTraits<T>::n_t> &NT = {},
    const std::optional<typename AliasInfoTraits<T>::v_t> &VT = {})
    -> decltype(std::make_tuple(
        CAI.isInterProcedural(), CAI.getAliasAnalysisType(),
        AI.alias(*VT, *VT, *NT), AI.getAliasSet(*VT, *NT),
        AI.getReachableAllocationSites(*VT, true, *NT),
        AI.isInReachableAllocationSites(*VT, *VT, true, *NT), CAI.getAsJson(),
        CAI.getAnalysisProperties(), CAI.isContextSensitive(),
        CAI.isFieldSensitive(), CAI.isFlowSensitive()));
template <typename T, typename = void, typename = void>
struct IsAliasInfo : std::false_type {};
template <typename T>
struct IsAliasInfo<
    T,
    std::void_t<decltype(std::declval<const T>().print(llvm::outs())),
                decltype(std::declval<const T>().printAsJson(llvm::outs())),
                decltype(std::declval<T>().mergeWith(std::declval<T>())),
                decltype(std::declval<T>().introduceAlias(
                    std::declval<typename AliasInfoTraits<T>::v_t>(),
                    std::declval<typename AliasInfoTraits<T>::v_t>(),
                    std::declval<typename AliasInfoTraits<T>::n_t>(),
                    AliasResult{}))>,
    std::enable_if_t<std::is_same_v<
        std::tuple<bool, AliasAnalysisType, AliasResult,
                   typename AliasInfoTraits<T>::AliasSetPtrTy,
                   typename AliasInfoTraits<T>::AllocationSiteSetPtrTy, bool,
                   nlohmann::json, AnalysisProperties, bool, bool, bool>,
        decltype(testAliasInfo(std::declval<T &>(),
                               std::declval<const T &>()))>>> : std::true_type {
};
} // namespace detail

template <typename T>
static constexpr bool IsAliasInfo = detail::IsAliasInfo<T>::value;

} // namespace psr

#endif // PHASAR_POINTER_ALIASINFOBASE_H
