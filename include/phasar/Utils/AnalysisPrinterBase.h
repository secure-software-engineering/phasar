#ifndef PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H
#define PHASAR_PHASARLLVM_UTILS_ANALYSISPRINTERBASE_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/Macros.h"

#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {

/// \brief A generic class that serves as the basis for a custom analysis
/// printer implementation.
template <typename AnalysisDomainTy> class AnalysisPrinterBase {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  template <typename D = d_t, typename L = l_t>
  void onResult(n_t Instr, D &&DfFact, L &&LatticeElement,
                DataFlowAnalysisType AnalysisType) {
    doOnResult(Instr, PSR_FWD(DfFact), PSR_FWD(LatticeElement), AnalysisType);
  }

  template <typename D = d_t>
  void onResult(n_t Instr, D &&DfFact, DataFlowAnalysisType AnalysisType)
    requires std::is_same_v<l_t, psr::BinaryDomain>
  {
    doOnResult(Instr, PSR_FWD(DfFact), psr::BinaryDomain::BOTTOM, AnalysisType);
  }

  void onInitialize() { doOnInitialize(); }

  void onFinalize() { doOnFinalize(); }
  void onFinalize(llvm::raw_ostream &OS) { doOnFinalize(OS); }

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
  virtual void doOnFinalize() { doOnFinalize(llvm::outs()); }
  virtual void doOnFinalize(llvm::raw_ostream &OS) {}
};

} // namespace psr

#endif
