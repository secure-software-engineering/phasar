#ifndef PHASAR_PHASALLVM_SIMPLEPROBLEMCONSTURCTOR_H
#define PHASAR_PHASALLVM_SIMPLEPROBLEMCONSTURCTOR_H

#include "phasar/PhasarLLVM/HelperAnalyses.h"

#include <type_traits>

namespace psr {
class LLVMProjectIRDB;
class LLVMPointsToInfo;
class LLVMBasedICFG;
class LLVMTypeHierarchy;

namespace detail {
template <typename ProblemTy, typename = void, typename... ArgTys>
struct HelperAnalysesSelector {};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<ProblemTy,
                              std::enable_if_t<std::is_constructible_v<
                                  ProblemTy, HelperAnalyses &, ArgTys...>>,
                              ArgTys...> {
  ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(HA, std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<
        std::is_constructible_v<ProblemTy, const LLVMProjectIRDB *, ArgTys...>>,
    ArgTys...> {
  ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<ProblemTy,
                              std::enable_if_t<std::is_constructible_v<
                                  ProblemTy, const LLVMProjectIRDB *,
                                  const LLVMPointsToInfo *, ArgTys...>>,
                              ArgTys...> {
  ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getPointsToInfo(),
                     std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<
        ProblemTy, const LLVMProjectIRDB *, const LLVMBasedICFG *,
        const LLVMPointsToInfo *, ArgTys...>>,
    ArgTys...> {
  ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
    return ProblemTy(&HA.getProjectIRDB(), &HA.getICFG(), &HA.getPointsToInfo(),
                     std::forward<ArgTys>(Args)...);
  }
};

template <typename ProblemTy, typename... ArgTys>
struct HelperAnalysesSelector<
    ProblemTy,
    std::enable_if_t<std::is_constructible_v<
        ProblemTy, const LLVMTypeHierarchy *, const LLVMProjectIRDB *,
        const LLVMBasedICFG *, const LLVMPointsToInfo *, ArgTys...>>,
    ArgTys...> {
  ProblemTy create(HelperAnalyses &HA, ArgTys... Args) {
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

#endif // PHASAR_PHASALLVM_SIMPLEPROBLEMCONSTURCTOR_H
