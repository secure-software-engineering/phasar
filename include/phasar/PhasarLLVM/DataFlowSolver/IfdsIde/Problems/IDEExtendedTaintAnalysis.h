/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEEXTENDEDTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEEXTENDEDTAINTANALYSIS_H_

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintAnalysisBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace psr {

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;

struct IDEExtendedTaintAnalysisDomain : public LLVMAnalysisDomainDefault {
  using d_t = AbstractMemoryLocation;
  /// Nullptr means tainted, nonnull llvm::Instruction* refers to a
  /// sanitizer on the current path, Bottom means sanitized on all paths.
  using l_t = XTaint::EdgeDomain;
};
namespace XTaint {

class IDEExtendedTaintAnalysis
    : public IDETabulationProblem<IDEExtendedTaintAnalysisDomain>,
      public AnalysisBase {
  using base_t = IDETabulationProblem<IDEExtendedTaintAnalysisDomain>;

public:
  using n_t =
      typename IDETabulationProblem<IDEExtendedTaintAnalysisDomain>::n_t;
  using f_t =
      typename IDETabulationProblem<IDEExtendedTaintAnalysisDomain>::f_t;
  using d_t =
      typename IDETabulationProblem<IDEExtendedTaintAnalysisDomain>::d_t;
  using l_t =
      typename IDETabulationProblem<IDEExtendedTaintAnalysisDomain>::l_t;
  using FlowFunctionPtrType = typename IDETabulationProblem<
      IDEExtendedTaintAnalysisDomain>::FlowFunctionPtrType;
  using EdgeFunctionPtrType = typename IDETabulationProblem<
      IDEExtendedTaintAnalysisDomain>::EdgeFunctionPtrType;

  // using FunctionInfoSetTy = XTaint::FunctionInfoSetTy;

  using config_callback_t = TaintConfig::TaintDescriptionCallBackTy;

private:
  struct SourceSinkInfo {
    llvm::SmallBitVector SourceIndices, SinkIndices;
  };

  // Helper functions

  /// Create a d_t from the given llvm::Value. Uses the
  /// AbstractMemoryLocationFactory to create an AbstractMemoryLocation for the
  /// given llvm::Value
  d_t makeFlowFact(const llvm::Value *V);
  /// Models an interprocedural flow, where the tainted value source flows to
  /// the callee via the actual parameter From that is mapped to the formal
  /// parameter To inside the callee.
  d_t transferFlowFact(d_t Source, d_t From, const llvm::Value *To);

  /// Add source to ret if it belongs to the same function as CurrInst. If
  /// addGlobals is true, also add llvm::GlobalValue.
  void identity(std::set<d_t> &Ret, const d_t &Source,
                const llvm::Instruction *CurrInst, bool AddGlobals = true);
  std::set<d_t> identity(const d_t &Source, const llvm::Instruction *CurrInst,
                         bool AddGlobals = true);

  [[nodiscard]] static inline bool equivalent(const d_t &LHS, const d_t &RHS) {
    return LHS->equivalent(RHS);
  }

  [[nodiscard]] static inline bool
  equivalentExceptPointerArithmetics(const d_t &LHS, const d_t &RHS) {
    return LHS->equivalentExceptPointerArithmetics(RHS);
  }

  /// Recursively walks the def-use chain of Inst to the first llvm::LoadInst,
  /// llvm::CallBase, llvm::AllocaInst or llvm::Argument and returns it.
  ///
  /// Used to identify the location since where the value of Inst is guaranteed
  /// not to change any more, i.e. not affected by sanitizers.
  const llvm::Instruction *getApproxLoadFrom(const llvm::Instruction *V) const;
  const llvm::Instruction *getApproxLoadFrom(const llvm::Value *V) const;

  /// A special flow-function factory for store-like instructions like
  /// llvm::StoreInst, llvm::MemSetInst, etc.
  FlowFunctionPtrType getStoreFF(const llvm::Value *PointerOp,
                                 const llvm::Value *ValueOp,
                                 const llvm::Instruction *Store,
                                 unsigned PALevel = 1);

  void populateWithMayAliases(SourceConfigTy &Facts) const;

  bool isMustAlias(const SanitizerConfigTy &Facts, d_t CurrNod);

  void generateFromZero(std::set<d_t> &Dest, const llvm::Instruction *Inst,
                        const llvm::Value *FormalArg,
                        const llvm::Value *ActualArg, bool IncludeActualArg);
  void reportLeakIfNecessary(const llvm::Instruction *Inst,
                             const llvm::Value *SinkCandidate,
                             const llvm::Value *LeakCandidate);

  FlowFunctionPtrType handleConfig(const llvm::Instruction *Inst,
                                   SourceConfigTy &&SourceConfig,
                                   SinkConfigTy &&SinkConfig);

  void doPostProcessing(const SolverResults<n_t, d_t, l_t> &SR);

public:
  /// Constructor. If EntryPoints is empty, use the TaintAPI functions as
  /// entrypoints.
  IDEExtendedTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                           const TaintConfig *TSF,
                           std::set<std::string> EntryPoints, unsigned Bound,
                           bool DisableStrongUpdates);

  ~IDEExtendedTaintAnalysis() override = default;

  // Flow functions

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallStmt, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               std::set<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallStmt,
                                             f_t DestFun) override;

  // Edge functions

  EdgeFunctionPtrType getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                            d_t SuccNode) override;

  EdgeFunctionPtrType getCallEdgeFunction(n_t CallInst, d_t SrcNode,
                                          f_t CalleeFun, d_t DestNode) override;

  EdgeFunctionPtrType getReturnEdgeFunction(n_t CallSite, f_t CalleeFun,
                                            n_t ExitInst, d_t ExitNode,
                                            n_t RetSite, d_t RetNode) override;

  EdgeFunctionPtrType getCallToRetEdgeFunction(n_t CallSite, d_t CallNode,
                                               n_t RetSite, d_t RetSiteNode,
                                               std::set<f_t> Callees) override;

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                             d_t SuccNode) override;

  // Misc

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const override;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  EdgeFunctionPtrType allTopFunction() override;

  // JoinLattice

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t LHS, l_t RHS) override;

  // Printing functions

  void printNode(std::ostream &OS, n_t Inst) const override;

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override;

  void printEdgeFact(std::ostream &OS, l_t Fact) const override;

  void printFunction(std::ostream &OS, f_t Fun) const override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      std::ostream &OS = std::cout) override;

private:
  /// Save all leaks here that were found using the IFDS part if the analysis.
  /// Hence, this map may contain sanitized facts.
  XTaint::LeakMap_t Leaks;

  // Used for determining whether a dataflow fact is still tained or already
  // sanitized
  BasicBlockOrdering BBO;

  AbstractMemoryLocationFactory<d_t> FactFactory;
  const llvm::DataLayout &DL;

#ifdef XTAINT_DIAGNOSTICS
  llvm::DenseSet<d_t> allTaintedValues;
#endif

  /// The k-limit for field-access paths
  unsigned Bound;

  /// Does the Leaks map still contain sanitized facts?
  bool PostProcessed = false;

  bool DisableStrongUpdates = false;

public:
  BasicBlockOrdering &getBasicBlockOrdering() { return BBO; }

  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may not be sanitized.
  ///
  /// This function involves a post-processing step the first time it is called.
  const LeakMap_t &
  getAllLeaks(IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) &;

  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may not be sanitized.
  ///
  /// This function involves a post-processing step the first time it is called.
  LeakMap_t getAllLeaks(IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) &&;
  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may or may not be sanitized.
  ///
  /// This function does NOT involve a post-processing step.
  LeakMap_t &getAllLeaks() { return Leaks; }

  [[nodiscard]] inline size_t getNumDataflowFacts() const {
    return FactFactory.size();
  }
#ifdef XTAINT_DIAGNOSTICS
  // Note: This number is probably smaller than getNumDataflowFacts()
  inline size_t getNumTaintedValues() const { return allTaintedValues.size(); }
  inline size_t getNumOverApproximatedFacts() const {
    return FactFactory.getNumOverApproximatedFacts();
  }
#endif
};

} // namespace XTaint

/// A Wrapper over XTaint::IDEExtendedTaintAnalysis that models the k-limit
/// (BOUND) as template parameter instead of a field.
template <unsigned BOUND = 3, bool USE_STRONG_UPDATES = true>
class IDEExtendedTaintAnalysis : public XTaint::IDEExtendedTaintAnalysis {
public:
  IDEExtendedTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                           const TaintConfig &TSF,
                           std::set<std::string> EntryPoints = {})
      : XTaint::IDEExtendedTaintAnalysis(IRDB, TH, ICF, PT, &TSF, EntryPoints,
                                         BOUND, !USE_STRONG_UPDATES) {}

  using ConfigurationTy = TaintConfig;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEEXTENDEDTAINTANALYSIS_H_