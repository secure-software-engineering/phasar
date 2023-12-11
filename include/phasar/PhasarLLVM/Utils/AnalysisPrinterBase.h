#ifndef PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H
#define PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H

#include "llvm/Support/raw_ostream.h"

namespace psr {

template <typename AnalysisDomainTy> struct Warning {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

  n_t Instr;
  d_t Fact;
  l_t LatticeElement;

  // Constructor
  Warning(n_t Inst, d_t DfFact, l_t Lattice)
      : Instr(std::move(Inst)), Fact(std::move(DfFact)),
        LatticeElement(std::move(Lattice)) {}
};

template <typename AnalysisDomainTy> struct DataflowAnalysisResults {
  std::vector<Warning<AnalysisDomainTy>> Warn;
};

template <typename AnalysisDomainTy> class AnalysisPrinterBase {
public:
  virtual void onResult(Warning<AnalysisDomainTy> /*Warn*/) = 0;
  virtual void onInitialize() = 0;
  virtual void onFinalize() const = 0;

  AnalysisPrinterBase() = default;
  virtual ~AnalysisPrinterBase() = default;
  AnalysisPrinterBase(const AnalysisPrinterBase &) = delete;
  AnalysisPrinterBase &operator=(const AnalysisPrinterBase &) = delete;

  AnalysisPrinterBase(AnalysisPrinterBase &&) = delete;
  AnalysisPrinterBase &operator=(AnalysisPrinterBase &&) = delete;
};

} // namespace psr

#endif
