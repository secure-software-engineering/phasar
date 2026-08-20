/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEEXTENDEDTAINTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEEXTENDEDTAINTANALYSIS_H

#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeProblemMixin.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintAnalysisBase.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/WithAnalysisPrinterMixin.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <concepts>
#include <functional>
#include <set>
#include <string>

namespace psr {

struct IDEExtendedTaintAnalysisDomain : public LLVMAnalysisDomainDefault {
  using d_t = AbstractMemoryLocation;
  /// Nullptr means tainted, nonnull llvm::Instruction* refers to a
  /// sanitizer on the current path, Bottom means sanitized on all paths.
  using l_t = XTaint::EdgeDomain;
};
namespace XTaint {

/// \brief An IDE-based taint analysis that uses k-limited field-access paths to
/// achieve field sensitivity
class IDEExtendedTaintAnalysis
    : public AnalysisBase,
      public IfdsIdeProblemMixin<IDEExtendedTaintAnalysisDomain>,
      public WithAnalysisPrinterMixin<IDEExtendedTaintAnalysisDomain> {
  using base_t = IfdsIdeProblemMixin<IDEExtendedTaintAnalysisDomain>;

public:
  using typename base_t::d_t;
  using typename base_t::f_t;
  using typename base_t::FlowFunctionPtrType;
  using typename base_t::l_t;
  using typename base_t::n_t;
  using EdgeFunctionType = EdgeFunction<l_t>;

  using config_callback_t = LLVMTaintConfig::TaintDescriptionCallBackTy;

  // Need SemiRing::identity; below static identity() conflicts
  using base_t::identity;

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
  static void identity(std::set<d_t> &Ret, d_t Source,
                       const llvm::Instruction *CurrInst,
                       bool AddGlobals = true);
  [[nodiscard]] static std::set<d_t> identity(d_t Source,
                                              const llvm::Instruction *CurrInst,
                                              bool AddGlobals = true);

  [[nodiscard]] static bool equivalent(d_t LHS, d_t RHS) {
    return LHS->equivalent(RHS);
  }

  [[nodiscard]] static bool equivalentExceptPointerArithmetics(d_t LHS,
                                                               d_t RHS) {
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
  std::set<d_t> propagateAtStore(AliasInfoRef<v_t, n_t>::AliasSetPtrTy PTS,
                                 d_t Source, d_t Val, d_t Mem,
                                 const llvm::Value *PointerOp,
                                 const llvm::Value *ValueOp,
                                 const llvm::Instruction *Store);

  void forEachAliasOf(AliasInfoRef<v_t, n_t>::AliasSetPtrTy PTS,
                      const llvm::Value *Of,
                      std::invocable<const llvm::Value *> auto &&CB) {
    if (!HasPreciseAliasInfo) {
      auto OfFF = makeFlowFact(Of);
      for (const auto *Alias : *PTS) {
        if (const auto *AliasGlob = llvm::dyn_cast<llvm::GlobalVariable>(Alias);
            AliasGlob && AliasGlob->isConstant()) {
          // Assume, data can never flow into the constant data section
          // Note: If a global constant is marked as source, it keeps being
          // propagated. We never assume, that the Of value is part of its
          // alias-set
          continue;
        }

        auto AliasFF = makeFlowFact(Alias);

        if (AliasFF->base() == OfFF->base() && AliasFF != OfFF) {
          continue;
        }

        std::invoke(CB, Alias);
      }
    } else {
      for (const auto *Alias : *PTS) {
        std::invoke(CB, Alias);
      }
    }
  }

  void populateWithMayAliases(SourceConfigTy &Facts);

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

  void doPostProcessing(GenericSolverResults<n_t, d_t, l_t> SR);

public:
  /// Constructor. If EntryPoints is empty, use the TaintAPI functions as
  /// entrypoints.
  /// The GetDomTree parameter can be used to inject a custom DominatorTree
  /// analysis or the results from a LLVM pass computing dominator trees
  template <typename GetDomTree = DefaultDominatorTreeAnalysis>
  IDEExtendedTaintAnalysis(const LLVMProjectIRDB *IRDB,
                           const LLVMBasedICFG *ICF, LLVMAliasInfoRef PT,
                           const LLVMTaintConfig *TSF,
                           std::vector<std::string> EntryPoints, unsigned Bound,
                           bool DisableStrongUpdates,
                           GetDomTree &&GDT = DefaultDominatorTreeAnalysis{})
      : AnalysisBase(TSF, IRDB->getNumInstructions()),
        base_t(IRDB, std::move(EntryPoints), createZeroValue()), PT(PT),
        ICF(ICF), BBO(std::forward<GetDomTree>(GDT)),
        DL(IRDB->getModule()->getDataLayout()), Bound(Bound),
        PostProcessed(DisableStrongUpdates),
        DisableStrongUpdates(DisableStrongUpdates) {
    assert(PT);
    assert(ICF != nullptr);

    FactFactory.setDataLayout(DL);

    getIFDSIDESolverConfig().setAutoAddZero(false);

    /// TODO: Once we have better AliasInfo, do a dynamic_cast over PT and
    /// set HasPreciseAliasInfo accordingly
  }

  // Flow functions

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ);

  FlowFunctionPtrType getCallFlowFunction(n_t CallStmt, f_t DestFun);

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite);

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               llvm::ArrayRef<f_t> Callees);

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallStmt, f_t DestFun);

  // Edge functions

  EdgeFunctionType getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                         d_t SuccNode);

  EdgeFunctionType getCallEdgeFunction(n_t CallInst, d_t SrcNode, f_t CalleeFun,
                                       d_t DestNode);

  EdgeFunctionType getReturnEdgeFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, d_t ExitNode,
                                         n_t RetSite, d_t RetNode);

  EdgeFunctionType getCallToRetEdgeFunction(n_t CallSite, d_t CallNode,
                                            n_t RetSite, d_t RetSiteNode,
                                            llvm::ArrayRef<f_t> Callees);

  EdgeFunctionType getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode);

  // Misc

  InitialSeeds<n_t, d_t, l_t> initialSeeds();

  [[nodiscard]] bool isZeroValue(d_t Fact) const noexcept;

  // Printing functions

  void emitTextReport(GenericSolverResults<n_t, d_t, l_t> SR,
                      llvm::raw_ostream &OS = llvm::outs());

  // ---

  [[nodiscard]] IFDSIDESolverConfig &getIFDSIDESolverConfig() noexcept {
    return SolverConfig;
  }

  BasicBlockOrdering &getBasicBlockOrdering() { return BBO; }

  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may not be sanitized.
  ///
  /// This function involves a post-processing step the first time it is called.
  const LeakMap_t &getAllLeaks(GenericSolverResults<n_t, d_t, l_t> SR) &;

  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may not be sanitized.
  ///
  /// This function involves a post-processing step the first time it is called.
  LeakMap_t getAllLeaks(GenericSolverResults<n_t, d_t, l_t> SR) &&;
  /// Return a map from llvm::Instruction to sets of leaks (llvm::Values) that
  /// may or may not be sanitized.
  ///
  /// This function does NOT involve a post-processing step.
  LeakMap_t &getAllLeaks() { return Leaks; }

  [[nodiscard]] size_t getNumDataflowFacts() const {
    return FactFactory.size();
  }
#ifdef XTAINT_DIAGNOSTICS
  // Note: This number is probably smaller than getNumDataflowFacts()
  inline size_t getNumTaintedValues() const { return allTaintedValues.size(); }
  inline size_t getNumOverApproximatedFacts() const {
    return FactFactory.getNumOverApproximatedFacts();
  }
#endif

private:
  LLVMAliasInfoRef PT{};
  const LLVMBasedICFG *ICF{};

  /// Save all leaks here that were found using the IFDS part if the analysis.
  /// Hence, this map may contain sanitized facts.
  XTaint::LeakMap_t Leaks;

  // Used for determining whether a dataflow fact is still tained or already
  // sanitized
  BasicBlockOrdering BBO;

  const llvm::DataLayout &DL;

#ifdef XTAINT_DIAGNOSTICS
  llvm::DenseSet<d_t> allTaintedValues;
#endif

  IFDSIDESolverConfig SolverConfig{};

  /// The k-limit for field-access paths
  unsigned Bound;

  /// Does the Leaks map still contain sanitized facts?
  bool PostProcessed = false;

  bool DisableStrongUpdates = false;

  bool HasPreciseAliasInfo = false;
};

} // namespace XTaint

/// A Wrapper over XTaint::IDEExtendedTaintAnalysis that models the k-limit
/// (BOUND) as template parameter instead of a field.
template <unsigned BOUND = 3, bool USE_STRONG_UPDATES = true>
class IDEExtendedTaintAnalysis : public XTaint::IDEExtendedTaintAnalysis {
public:
  template <typename GetDomTree = DefaultDominatorTreeAnalysis>
  IDEExtendedTaintAnalysis(const LLVMProjectIRDB *IRDB,
                           const LLVMBasedICFG *ICF, LLVMAliasInfoRef PT,
                           const LLVMTaintConfig &TSF,
                           std::vector<std::string> EntryPoints = {},
                           GetDomTree &&GDT = DefaultDominatorTreeAnalysis{})
      : XTaint::IDEExtendedTaintAnalysis(
            IRDB, ICF, PT, &TSF, std::move(EntryPoints), BOUND,
            !USE_STRONG_UPDATES, std::forward<GetDomTree>(GDT)) {}

  using ConfigurationTy = LLVMTaintConfig;
};

} // namespace psr

#endif
