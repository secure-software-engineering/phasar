#ifndef PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H
#define PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H

#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

#include "llvm/Support/raw_ostream.h"

namespace psr {

template <typename AnalysisDomainTy> class AnalysisPrinterBase {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  /// TODO: use non-virtual function to call virtual function with default
  /// parameters +
  /// TODO: templace magic - #include "memory_resource"
  virtual void onResult(n_t /*Instr*/, d_t /*DfFact*/, l_t /*LatticeElement*/,
                        DataFlowAnalysisType /*AnalysisType*/) = 0;
  virtual void onInitialize() = 0;
  virtual void onFinalize() = 0;

  AnalysisPrinterBase() = default;
  virtual ~AnalysisPrinterBase() = default;
  AnalysisPrinterBase(const AnalysisPrinterBase &) = delete;
  AnalysisPrinterBase &operator=(const AnalysisPrinterBase &) = delete;

  AnalysisPrinterBase(AnalysisPrinterBase &&) = delete;
  AnalysisPrinterBase &operator=(AnalysisPrinterBase &&) = delete;
};

} // namespace psr

#endif
