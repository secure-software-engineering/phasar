#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSProblem.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h" // for concept checking
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMAnalysisPrinter.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NullAnalysisPrinter.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/UsedGlobalsHolder.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <memory_resource>
#include <type_traits>

namespace psr::monoifds {

/// Implementation of a generic taint analysis to be solved by the
/// MonoIFDSSolver. Conforms to the MonoIFDSProblem concept.
class TaintAnalysis : public LLVMIFDSAnalysisDomainDefault {
public:
  using ProblemAnalysisDomain = LLVMIFDSAnalysisDomainDefault;

  TaintAnalysis(
      const LLVMTaintConfig *Config,
      const UsedGlobalsHolder<const llvm::GlobalVariable *> *UsedGlobals,
      LLVMAliasIteratorRef AI)
      : Config(&assertNotNull(Config)),
        UsedGlobals(&assertNotNull(UsedGlobals)), AI(AI) {
    static_assert(MonoIFDSProblem<TaintAnalysis>);
  }

  void setAnalysisPrinter(
      MaybeUniquePtr<AnalysisPrinterBase<ProblemAnalysisDomain>> P) {
    if (P) {
      Printer = std::move(P);
    } else {
      Printer = NullAnalysisPrinter<ProblemAnalysisDomain>::getInstance();
    }
  }

  struct LocalAnalysis {
    TaintAnalysis *TA{};
    AliasCache AC;
    SCCId<FunctionId> CurrSCC;

    void normalFlow(DataFlowEnvironment<d_t> &InOut, n_t Curr);
    void callToRetFlow(DataFlowEnvironment<d_t> &InOut, n_t Curr);
    [[nodiscard]] llvm::SmallVector<d_t> returnFlow(n_t CallSite, d_t Fact);
    [[nodiscard]] llvm::SmallVector<d_t> invCallFlow(n_t CallSite, d_t Fact);
    [[nodiscard]] std::false_type
    summaryFlow(const DataFlowEnvironment<d_t> & /*In*/,
                DataFlowEnvironment<d_t> & /*Out*/, n_t /*Curr*/,
                f_t /*Callee*/) {
      // No propagators defined so far
      return {};
    }

    [[nodiscard]] d_t getZeroValue() const {
      return LLVMZeroValue::getInstance();
    }

    void initialSeeds(DataFlowEnvironment<d_t> &SeedState,
                      Compressor<d_t, SourceFactId> &SeedCompressor, f_t Fun);

    void generateFactsAtCall(n_t CS, f_t Callee,
                             llvm::function_ref<void(d_t)> GenFact);
    void generateFacts(n_t CS, llvm::function_ref<void(d_t)> GenFact) {
      // XXX: Implement (was not necessary for paper eval)
    }
    void
    requestResultCallbackAtCallSite(n_t CS, f_t Callee,
                                    llvm::function_ref<void(d_t)> LeakFact);
    void requestResultCallback(n_t Inst,
                               llvm::function_ref<void(d_t)> LeakFact) {
      // XXX: Implement (was not necessary for paper eval)
    }
    void onResult(n_t Inst, d_t Fact) {
      TA->Printer->onResult(Inst, Fact,
                            DataFlowAnalysisType::IFDSTaintAnalysis);
    }
  };

  [[nodiscard]] LocalAnalysis localAnalysis(SCCId<FunctionId> CurrSCC,
                                            std::pmr::memory_resource *MRes) {
    return LocalAnalysis{
        .TA = this,
        .AC = AliasCache(AI,
                         Config->getRegisteredSkipSeedsCallBack()
                             ? (llvm::function_ref<bool(const llvm::Value *)>)
                                   Config->getRegisteredSkipSeedsCallBack()
                             : llvm::function_ref<bool(const llvm::Value *)>{},
                         &UsedGlobals->GlobsPerSCC[CurrSCC], MRes),
        .CurrSCC = CurrSCC,
    };
  }

  void emitTextReport(llvm::raw_ostream &OS) const {
    OS << "\n----- Found the following leaks -----\n";
    Printer->onFinalize(OS);
  }

  // Optional API function: Filter out facts that do not need to go into a
  // procedure summary
  [[nodiscard]] bool shouldBeInSummary(d_t ExitFact, n_t ExitInst);

private:
  MaybeUniquePtr<AnalysisPrinterBase<ProblemAnalysisDomain>> Printer =
      std::make_unique<DefaultLLVMAnalysisPrinter<ProblemAnalysisDomain>>();
  const LLVMTaintConfig *Config{};
  const UsedGlobalsHolder<const llvm::GlobalVariable *> *UsedGlobals{};
  LLVMAliasIteratorRef AI;
};
} // namespace psr::monoifds
