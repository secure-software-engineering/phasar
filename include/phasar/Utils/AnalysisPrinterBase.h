#ifndef PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H
#define PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {

template <typename AnalysisDomainTy> class AnalysisPrinterBase {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  template <typename L = l_t, typename = std::enable_if_t<!std::is_same_v<
                                  std::decay_t<L>, psr::BinaryDomain>>>
  void onResult(n_t Instr, d_t DfFact, l_t LatticeElement,
                DataFlowAnalysisType AnalysisType) {
    doOnResult(Instr, DfFact, LatticeElement, AnalysisType);
  }

  template <typename L = l_t, typename = std::enable_if_t<std::is_same_v<
                                  std::decay_t<L>, psr::BinaryDomain>>>
  void onResult(n_t Instr, d_t DfFact, DataFlowAnalysisType AnalysisType) {
    doOnResult(Instr, DfFact, psr::BinaryDomain::BOTTOM, AnalysisType);
  }

  void onInitialize() { doOnInitialize(); }

  void onFinalize() { doOnFinalize(); }

  AnalysisPrinterBase() = default;
  virtual ~AnalysisPrinterBase() = default;
  AnalysisPrinterBase(const AnalysisPrinterBase &) = delete;
  AnalysisPrinterBase &operator=(const AnalysisPrinterBase &) = delete;

  AnalysisPrinterBase(AnalysisPrinterBase &&) = delete;
  AnalysisPrinterBase &operator=(AnalysisPrinterBase &&) = delete;

private:
  virtual void doOnResult(n_t /*Instr*/, d_t /*DfFact*/, l_t /*LatticeElement*/,
                          DataFlowAnalysisType /*AnalysisType*/) = 0;

  virtual void doOnInitialize() {}
  virtual void doOnFinalize() {}
};

} // namespace psr

#endif
