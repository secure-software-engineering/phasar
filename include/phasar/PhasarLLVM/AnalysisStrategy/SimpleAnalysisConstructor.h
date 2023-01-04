/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_SIMPLEANALYSISCONSTRUCTOR_H_
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_SIMPLEANALYSISCONSTRUCTOR_H_

#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"

#include <type_traits>
#include <utility>
namespace psr {
class LLVMProjectIRDB;
class LLVMPointsToInfo;
class LLVMBasedICFG;
class LLVMTypeHierarchy;

namespace detail {
template <typename ProblemTy, typename Enable, typename... ArgTys>
struct HelperAnalysesSelector {};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<ProblemTy,
                              std::enable_if_t<std::is_constructible_v<
                                  ProblemTy, HelperAnalyses &, ArgTys...>>,
                              ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(HA, std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<
        std::is_constructible_v<ProblemTy, const LLVMProjectIRDB *, ArgTys...>>,
    ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<ProblemTy, const LLVMProjectIRDB *,
                                             const LLVMBasedICFG *, ArgTys...>>,
    ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getICFG(),
                     std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<ProblemTy, const LLVMProjectIRDB *,
                                             LLVMPointsToInfo *, ArgTys...>>,
    ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getPointsToInfo(),
                     std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<ProblemTy, const LLVMProjectIRDB *,
                                             const LLVMBasedICFG *,
                                             LLVMPointsToInfo *, ArgTys...>>,
    ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getICFG(), &HA.getPointsToInfo(),
                     std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<
        ProblemTy, const LLVMProjectIRDB *, const LLVMTypeHierarchy *,
        const LLVMBasedICFG *, LLVMPointsToInfo *, ArgTys...>>,
    ArgTys...> {
  [[nodiscard]] static ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getTypeHierarchy(),
                     &HA.getICFG(), &HA.getPointsToInfo(),
                     std::forward<ArgTys>(Args)...);
  }
};

} // namespace detail

template <typename ProblemTy, typename... ArgTys>
ProblemTy createAnalysisProblem(HelperAnalyses &HA, ArgTys &&...Args) {
  return detail::HelperAnalysesSelector<ProblemTy, void, ArgTys...>::create(
      HA, std::forward<ArgTys>(Args)...);
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_ANALYSISSTRATEGY_SIMPLEANALYSISCONSTRUCTOR_H_
