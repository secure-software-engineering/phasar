/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace psr {

/// \file Implements field-sensitivity after the paper "Boosting the performance
/// of alias-aware IFDS analysis with CFL-based environment transformers" by Li
/// et al.

struct StoreEvent {};
struct LoadEvent {};

struct KillEvent {};

struct GEPEvent {
  int32_t Field;
};

struct CFLFieldAccessPath {
  static constexpr int32_t TopOffset = INT32_MIN;

  // TODO: compose, DenseMapInfo

  llvm::SmallVector<int32_t, 4> Loads;
  llvm::SmallVector<int32_t, 4> Stores;
  llvm::SmallDenseSet<int32_t, 2> Kills;
  // Add an offset for pending GEPs; INT32_MIN is Top
  int32_t Offset = {0};
  int32_t EmptyTombstone = 0;

  [[nodiscard]] bool kills(int32_t Off) const {
    return Off != TopOffset && Kills.count(Off);
  }

  [[nodiscard]] bool
  operator==(const CFLFieldAccessPath &Other) const noexcept {
    return EmptyTombstone == Other.EmptyTombstone && Loads == Other.Loads &&
           Stores == Other.Stores && Kills == Other.Kills;
  }

  bool operator!=(const CFLFieldAccessPath &Other) const noexcept {
    return !(*this == Other);
  }

  friend size_t hash_value(const CFLFieldAccessPath &FieldString) noexcept;
};

struct CFLFieldAccessPathDMI {
  static CFLFieldAccessPath getEmptyKey() {
    CFLFieldAccessPath Ret{};
    Ret.EmptyTombstone = 1;
    return Ret;
  }
  static CFLFieldAccessPath getTombstoneKey() {
    CFLFieldAccessPath Ret{};
    Ret.EmptyTombstone = 2;
    return Ret;
  }
  static auto getHashValue(const CFLFieldAccessPath &FieldString) noexcept {
    return hash_value(FieldString);
  }
  static bool isEqual(const CFLFieldAccessPath &L,
                      const CFLFieldAccessPath &R) noexcept {
    return L == R;
  }
};

struct CFLFieldSensEdgeValue {
  // TODO: JoinLatticeTraits

  llvm::SmallDenseSet<CFLFieldAccessPath, 2, CFLFieldAccessPathDMI> Paths;

  void applyStore();
  void applyLoad();
  void applyKill();
  void applyGep(GEPEvent Evt);
};

template <typename AnalysisDomainTy>
struct CFLFieldSensAnalysisDomain : AnalysisDomainTy {
  using l_t = LatticeDomain<CFLFieldSensEdgeValue>;
};

class FieldSensAllocSitesAwareIFDSProblem
    : public IDETabulationProblem<
          CFLFieldSensAnalysisDomain<LLVMIFDSAnalysisDomainDefault>> {
  using Base = IDETabulationProblem<
      CFLFieldSensAnalysisDomain<LLVMIFDSAnalysisDomainDefault>>;

public:
  using typename Base::container_type;
  using typename Base::d_t;
  using typename Base::db_t;
  using typename Base::f_t;
  using typename Base::FlowFunctionPtrType;
  using typename Base::i_t;
  using typename Base::l_t;
  using typename Base::n_t;
  using typename Base::ProblemAnalysisDomain;
  using typename Base::t_t;
  using typename Base::v_t;

  // Constructs an IDETabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit FieldSensAllocSitesAwareIFDSProblem(
      IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem,
      LLVMAliasInfoRef AS) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : Base(UserProblem->getProjectIRDB(), UserProblem->getEntryPoints(),
             UserProblem->getZeroValue()),
        AS(AS), UserProblem(UserProblem) {}

  FieldSensAllocSitesAwareIFDSProblem(std::nullptr_t,
                                      LLVMAliasInfoRef AS) = delete;

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr,
                                                          n_t Succ) override {
    return UserProblem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return UserProblem->getCallFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType
  getSummaryFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return UserProblem->getSummaryFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType getRetFlowFunction(n_t CallSite,
                                                       f_t CalleeFun,
                                                       n_t ExitInst,
                                                       n_t RetSite) override {
    return UserProblem->getRetFlowFunction(CallSite, CalleeFun, ExitInst,
                                           RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    return UserProblem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override;

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction,
                                        d_t DestNode) override;

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitStmt, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override;

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                           d_t SuccNode) override;

  EdgeFunction<l_t> extend(const EdgeFunction<l_t> &L,
                           const EdgeFunction<l_t> &R) override;

  EdgeFunction<l_t> combine(const EdgeFunction<l_t> &L,
                            const EdgeFunction<l_t> &R) override;

private:
  LLVMAliasInfoRef AS;
  IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
