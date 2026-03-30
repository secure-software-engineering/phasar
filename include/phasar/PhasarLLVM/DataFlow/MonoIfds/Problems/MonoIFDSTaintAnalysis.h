#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMAnalysisPrinter.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionCompressor.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NullAnalysisPrinter.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/UsedGlobalsHolder.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <memory_resource>

namespace psr::monoifds {
class TaintAnalysis : public LLVMIFDSAnalysisDomainDefault {
public:
  using ProblemAnalysisDomain = LLVMIFDSAnalysisDomainDefault;

  TaintAnalysis(
      const LLVMTaintConfig *Config,
      const UsedGlobalsHolder<const llvm::GlobalVariable *> *UsedGlobals,
      LLVMAliasIteratorRef AI)
      : Config(&assertNotNull(Config)),
        UsedGlobals(&assertNotNull(UsedGlobals)), AI(AI) {}

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
    [[nodiscard]] llvm::SmallVector<d_t> invReturnFlow(n_t CallSite, d_t Fact);

    [[nodiscard]] d_t getZeroValue() const {
      return LLVMZeroValue::getInstance();
    }

    void initialSeeds(DataFlowEnvironment<d_t> &SeedState,
                      Compressor<d_t, SourceFactId> &SeedCompressor, f_t Fun);

    void generateTaintsAtCall(n_t CS, f_t Callee,
                              llvm::function_ref<void(d_t)> GenFact);
    void generateTaints(n_t CS, llvm::function_ref<void(d_t)> GenFact) {
      // XXX: Implement (was not necessary for paper eval)
    }
    void leakTaintsAtCall(n_t CS, f_t Callee,
                          llvm::function_ref<void(d_t)> LeakFact);
    void leakTaints(n_t CS, llvm::function_ref<void(d_t)> LeakFact) {
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
        .AC = AliasCache(AI, Config->getRegisteredSkipSeedsCallBack(),
                         &UsedGlobals->GlobsPerSCC[CurrSCC], MRes),
        .CurrSCC = CurrSCC,
    };
  }

  // TODO: shouldBeInSummary()

private:
  MaybeUniquePtr<AnalysisPrinterBase<ProblemAnalysisDomain>> Printer =
      std::make_unique<DefaultLLVMAnalysisPrinter<ProblemAnalysisDomain>>();
  const LLVMTaintConfig *Config{};
  const UsedGlobalsHolder<const llvm::GlobalVariable *> *UsedGlobals{};
  LLVMAliasIteratorRef AI;
};
} // namespace psr::monoifds
