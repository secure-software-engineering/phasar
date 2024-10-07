/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_SIMPLEANALYSISCONSTRUCTOR_H
#define PHASAR_PHASARLLVM_SIMPLEANALYSISCONSTRUCTOR_H

#include "phasar/PhasarLLVM/HelperAnalyses.h"

#include <type_traits>
#include <utility>

namespace psr {
class LLVMProjectIRDB;
class LLVMAliasSet;
class LLVMBasedICFG;
class DIBasedTypeHierarchy;

template <typename ProblemTy, typename... ArgTys>
ProblemTy createAnalysisProblem(HelperAnalyses &HA, ArgTys &&...Args) {
  if constexpr (std::is_constructible_v<ProblemTy, HelperAnalyses &,
                                        ArgTys...>) {
    return ProblemTy(HA, std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<
                           ProblemTy, const LLVMProjectIRDB *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<
                           ProblemTy, const LLVMProjectIRDB *,
                           const LLVMBasedICFG *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getICFG(),
                     std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<ProblemTy,
                                               const LLVMProjectIRDB *,
                                               LLVMAliasSet *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getAliasInfo(),
                     std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<
                           ProblemTy, const LLVMProjectIRDB *,
                           const LLVMBasedICFG *, LLVMAliasSet *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getICFG(), &HA.getAliasInfo(),
                     std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<
                           ProblemTy, const LLVMProjectIRDB *,
                           const DIBasedTypeHierarchy *, const LLVMBasedCFG *,
                           LLVMAliasSet *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getTypeHierarchy(), &HA.getCFG(),
                     &HA.getAliasInfo(), std::forward<ArgTys>(Args)...);
  } else if constexpr (std::is_constructible_v<
                           ProblemTy, const LLVMProjectIRDB *,
                           const DIBasedTypeHierarchy *, const LLVMBasedICFG *,
                           LLVMAliasSet *, ArgTys...>) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getTypeHierarchy(),
                     &HA.getICFG(), &HA.getAliasInfo(),
                     std::forward<ArgTys>(Args)...);
  } else {
    static_assert(
        std::is_constructible_v<ProblemTy, HelperAnalyses &, ArgTys...>,
        "Cannot construct analysis problem from HelperAnalyses");
  }
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_SIMPLEANALYSISCONSTRUCTOR_H
