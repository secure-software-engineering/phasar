#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H

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
  static constexpr bool IsSimple1d =
      sizeof(std::pair<K, V>) <= 32 &&
      std::is_nothrow_move_constructible_v<K> &&
      std::is_nothrow_move_constructible_v<V> && has_llvm_dense_map_info<K>;

  template <typename T>
  static constexpr bool IsSimpleVal =
      sizeof(T) <= 32 && std::is_nothrow_move_constructible_v<T> &&
      has_llvm_dense_map_info<T>;

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

  static constexpr bool AutoAddZero = true;
  static constexpr bool EnableStatistics = false;
  static constexpr JumpFunctionGCMode EnableJumpFunctionGC =
      JumpFunctionGCMode::Disabled;
  static constexpr bool UseEndSummaryTab = true;
};

template <typename Base, bool ComputeValuesVal> struct WithComputeValues;
using IDESolverConfig = WithComputeValues<IDESolverConfigBase, true>;
using IFDSSolverConfig = WithComputeValues<IDESolverConfigBase, false>;

template <typename Base, bool ComputeValuesVal>
struct [[clang::preferred_name(IDESolverConfig),
         clang::preferred_name(IFDSSolverConfig)]]
WithComputeValues : Base {
  static constexpr bool ComputeValues = ComputeValuesVal;
};

template <typename Base, bool EnableStats> struct WithStats;
using IDESolverConfigWithStats = WithStats<IDESolverConfig, true>;
using IFDSSolverConfigWithStats = WithStats<IFDSSolverConfig, true>;

template <typename Base, bool EnableStats>
struct [[clang::preferred_name(IDESolverConfigWithStats),
         clang::preferred_name(IFDSSolverConfigWithStats)]] WithStats : Base {
  static constexpr bool EnableStatistics = EnableStats;
};

template <typename Base, template <typename> typename WorkList>
struct WithWorkList : Base {
  template <typename T> using worklist_t = WorkList<T>;
};

template <typename Base, bool UseEST> struct WithEndSummaryTab : Base {
  static constexpr bool UseEndSummaryTab = UseEST;
};

template <typename Base, JumpFunctionGCMode GCMode> struct WithGCMode;

using IFDSSolverConfigWithStatsAndGC =
    WithGCMode<IFDSSolverConfigWithStats, JumpFunctionGCMode::Enabled>;
template <typename Base, JumpFunctionGCMode GCMode>
struct [[clang::preferred_name(IFDSSolverConfigWithStatsAndGC)]] WithGCMode
    : Base {
  static constexpr JumpFunctionGCMode EnableJumpFunctionGC = GCMode;
};

template <typename ProblemTy>
struct DefaultIDESolverConfig : IDESolverConfig {};

template <typename ProblemTy>
  requires std::is_base_of_v<
      IFDSTabulationProblem<typename ProblemTy::ProblemAnalysisDomain>,
      ProblemTy>
struct DefaultIDESolverConfig<ProblemTy> : IFDSSolverConfig {};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_STATICIDESOLVERCONFIG_H
