#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERBASE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERBASE_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/PointerUtils.h"
#include "phasar/Utils/TableWrappers.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"

#include <type_traits>

namespace psr {

template <typename EdgeFunctionPtrType> struct IterIDEPropagationJob {
  [[no_unique_address]] EdgeFunctionPtrType SourceEF{};

  uint32_t AtInstruction{};
  uint32_t SourceFact{};
  uint32_t PropagatedFact{};
};

template <typename StaticSolverConfigTy, typename EdgeFunPtrTy>
class IterativeIDESolverBase {
public:
  static constexpr bool ComputeValues = StaticSolverConfigTy::ComputeValues;
  static constexpr bool EnableStatistics =
      StaticSolverConfigTy::EnableStatistics;
  /// NOTE: EdgeFunctionPtrType may be either std::shared_ptr<EdgeFunction<l_t>>
  /// or llvm::IntrusiveRefCntPtr<EdgeFunction<l_t>> once this is supported
  using EdgeFunctionPtrType =
      std::conditional_t<ComputeValues, EdgeFunPtrTy, EmptyType>;

protected:
  template <typename K, typename V>
  using map_t = typename StaticSolverConfigTy::template map_t<K, V>;

  template <typename T>
  using set_t = typename StaticSolverConfigTy::template set_t<T>;

  template <typename T>
  using worklist_t = typename StaticSolverConfigTy::template worklist_t<T>;

  template <typename ProblemTy, bool AutoAddZero>
  using flow_edge_function_cache_t =
      typename StaticSolverConfigTy::template flow_edge_function_cache_t<
          ProblemTy, AutoAddZero>;

  using PropagationJob = IterIDEPropagationJob<EdgeFunctionPtrType>;

  struct InterPropagationJob {
    [[no_unique_address]] EdgeFunctionPtrType SourceEF{};

    uint32_t SourceFact{};
    uint32_t Callee{};

    uint32_t CallSite{};
    uint32_t FactInCallee{};

    /// NOTE: The Next-pointer must be mutable, such that we are able to mutate
    /// it from inside a set. Recall that the next pointer does *not* affect
    /// equality or hashing
    mutable const InterPropagationJob *NextWithSameSourceFactAndCallee{};
    [[no_unique_address]] mutable std::conditional_t<
        ComputeValues, const InterPropagationJob *, EmptyType>
        NextWithSameSourceFactAndCS{};

    [[nodiscard]] bool
    operator==(const InterPropagationJob &Other) const noexcept {
      return SourceFact == Other.SourceFact && Callee == Other.Callee &&
             CallSite == Other.CallSite && FactInCallee == Other.FactInCallee &&
             SourceEF == Other.SourceEF;
    }

    [[nodiscard]] bool
    operator!=(const InterPropagationJob &Other) const noexcept {
      return !(*this == Other);
    }

    [[nodiscard]] size_t getHashCode() const noexcept {
      if constexpr (ComputeValues) {
        return llvm::hash_combine(SourceFact, Callee, CallSite, FactInCallee,
                                  SourceEF.getOpaqueValue());
      } else {
        return llvm::hash_combine(SourceFact, Callee, CallSite, FactInCallee);
      }
    }
  };
  struct InterPropagationJobRef {
    InterPropagationJob *JobPtr{};
  };

  struct InterPropagationJobRefDSI {
    static InterPropagationJobRef getEmptyKey() noexcept {
      return {llvm::DenseMapInfo<InterPropagationJob *>::getEmptyKey()};
    }
    static InterPropagationJobRef getTombstoneKey() noexcept {
      return {llvm::DenseMapInfo<InterPropagationJob *>::getTombstoneKey()};
    }
    static auto getHashValue(InterPropagationJobRef Job) noexcept {
      assert(Job.JobPtr != nullptr);
      assert(Job.JobPtr != getEmptyKey().JobPtr);
      assert(Job.JobPtr != getTombstoneKey().JobPtr);
      return Job.JobPtr->getHashCode();
    }
    static bool isEqual(InterPropagationJobRef LHS,
                        InterPropagationJobRef RHS) noexcept {
      if (LHS.JobPtr == RHS.JobPtr) {
        return true;
      }
      if (LHS.JobPtr == getEmptyKey().JobPtr ||
          LHS.JobPtr == getTombstoneKey().JobPtr ||
          RHS.JobPtr == getEmptyKey().JobPtr ||
          RHS.JobPtr == getTombstoneKey().JobPtr) {
        return false;
      }

      assert(LHS.JobPtr != nullptr);
      assert(RHS.JobPtr != nullptr);
      return *LHS.JobPtr == *RHS.JobPtr;
    }
  };

  union ValueComputationJob {
    uint64_t Value{};
    struct {
      uint32_t StartPoint;
      uint32_t SourceFact;
    };

    ValueComputationJob(uint64_t Value) noexcept : Value(Value) {}
    ValueComputationJob(uint32_t SP, uint32_t SF) noexcept
        : StartPoint(SP), SourceFact(SF) {}
  };

  struct ValuePropagationJob {
    uint32_t Inst{};
    uint32_t Fact{};
    [[no_unique_address]] std::conditional_t<
        ComputeValues, typename std::decay_t<EdgeFunPtrTy>::l_t, EmptyType>
        Value{};
  };

  struct SummaryEdge {
    uint32_t TargetFact{};
    EdgeFunctionPtrType EF{};
  };

  /// Key is TargetFact

  using SummaryEdges = SmallDenseTable1d<uint64_t, EdgeFunctionPtrType, 4>;
  using SummaryEdges_JF1 =
      std::conditional_t<ComputeValues,
                         llvm::SmallDenseMap<uint32_t, EdgeFunctionPtrType>,
                         llvm::SmallDenseSet<DummyPair<uint32_t>>>;

  using summaries_t = detail::CellVecSmallVectorTy<std::conditional_t<
      ComputeValues, std::pair<uint32_t, EdgeFunctionPtrType>,
      DummyPair<uint32_t>>>;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERBASE_H
