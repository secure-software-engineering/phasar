#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheNG.h"
#include "phasar/DataFlow/IfdsIde/Solver/WorkListTraits.h"
#include "phasar/Utils/TableWrappers.h"
#include "phasar/Utils/TypeTraits.h"

#include <type_traits>
#include <utility>

namespace psr {
enum class JumpFunctionGCMode {
  /// Perform no semantic garbage collection on jump functions
  Disabled,
  /// Perform JF GC, but do not delete any elements from the SolverResults, so
  /// all SolverResults are still present, only the edge-values are not computed
  /// everywhere
  Enabled,
  /// Perform JF GC as in 'Enabled', but also delete entries from the
  /// SolverResults
  EnabledAggressively
};

struct IDESolverConfigBase {
  template <typename K, typename V>
  static inline constexpr bool
      IsSimple1d = sizeof(std::pair<K, V>) <= 32 &&
                   std::is_nothrow_move_constructible_v<K>
                       &&std::is_nothrow_move_constructible_v<V>
                           &&has_llvm_dense_map_info<K>;

  template <typename T>
  static inline constexpr bool
      IsSimpleVal = sizeof(T) <= 32 && std::is_nothrow_move_constructible_v<T>
                                           &&has_llvm_dense_map_info<T>;

  template <typename K, typename V>
  using map_t = std::conditional_t<IsSimple1d<K, V>, DenseTable1d<K, V>,
                                   UnorderedTable1d<K, V>>;

  template <typename T>
  using set_t =
      std::conditional_t<IsSimpleVal<T>, DenseSet<T>, UnorderedSet<T>>;

  template <typename T> using worklist_t = VectorWorkList<T>;

  template <typename ProblemTy, bool AutoAddZero>
  using flow_edge_function_cache_t =
      FlowEdgeFunctionCacheNG<ProblemTy, AutoAddZero>;

  template <typename L> using EdgeFunctionPtrType = EdgeFunction<L>;

  static inline constexpr bool AutoAddZero = true;
  static inline constexpr bool EnableStatistics = false;
  static inline constexpr JumpFunctionGCMode EnableJumpFunctionGC =
      JumpFunctionGCMode::Disabled;
  static inline constexpr bool UseEndSummaryTab = false;
};

template <typename Base, bool ComputeValuesVal>
struct WithComputeValues : Base {
  static constexpr bool ComputeValues = ComputeValuesVal;
};

template <typename Base, JumpFunctionGCMode GCMode> struct WithGCMode : Base {
  static constexpr JumpFunctionGCMode EnableJumpFunctionGC = GCMode;
};

template <typename Base, bool EnableStats> struct WithStats : Base {
  static constexpr bool EnableStatistics = EnableStats;
};

template <typename Base, template <typename> typename WorkList>
struct WithWorkList : Base {
  template <typename T> using worklist_t = WorkList<T>;
};

template <typename Base, bool UseEST> struct WithEndSummaryTab : Base {
  static inline constexpr bool UseEndSummaryTab = UseEST;
};

using IDESolverConfig = WithComputeValues<IDESolverConfigBase, true>;
using IFDSSolverConfig = WithComputeValues<IDESolverConfigBase, false>;
using IDESolverConfigWithStats = WithStats<IDESolverConfig, true>;
using IFDSSolverConfigWithStats = WithStats<IFDSSolverConfig, true>;
using IFDSSolverConfigWithStatsAndGC =
    WithGCMode<IFDSSolverConfigWithStats, JumpFunctionGCMode::Enabled>;

template <typename ProblemTy, typename Enable = void>
struct DefaultIDESolverConfig : IDESolverConfig {};
// template <typename ProblemTy>
// struct DefaultIDESolverConfig<
//     ProblemTy,
//     std::enable_if_t<std::is_base_of_v<
//         IFDSTabulationProblemLight<
//             typename ProblemTy::ProblemAnalysisDomain::BaseAnalysisDomain,
//             ProblemTy>,
//         ProblemTy>>> : IFDSSolverConfig {};

template <typename ProblemTy>
struct DefaultIDESolverConfig<
    ProblemTy,
    std::enable_if_t<std::is_base_of_v<
        IFDSTabulationProblem<typename ProblemTy::ProblemAnalysisDomain>,
        ProblemTy>>> : IFDSSolverConfig {};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H
