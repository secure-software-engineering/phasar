#ifndef PHASAR_PHASARLLVM_UTILS_NULLANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_NULLANALYSISPRINTER_H

#include "phasar/Utils/AnalysisPrinterBase.h"

namespace psr {

template <typename AnalysisDomainTy>
class NullAnalysisPrinter final : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  static NullAnalysisPrinter *getInstance() {
    static auto Instance = NullAnalysisPrinter();
    return &Instance;
  }

private:
  void doOnResult(n_t /*Instr*/, d_t /*DfFact*/, l_t /*Lattice*/,
                  DataFlowAnalysisType /*AnalysisType*/) override{};

  NullAnalysisPrinter() = default;
};

} // namespace psr
#endif
