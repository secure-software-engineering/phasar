#ifndef PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Printer.h"

#include "llvm/Support/raw_ostream.h"

#include <type_traits>
#include <vector>

namespace psr {

template <typename AnalysisDomainTy> struct Warning {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

  n_t Instr;
  d_t Fact;
  l_t LatticeElement;
  DataFlowAnalysisType AnalysisType;

  // Constructor -- With C++20, we can get rid of it; then the below
  // emplace_back works on aggregates too
  constexpr Warning(n_t Inst, ByMoveRef<d_t> DfFact, ByMoveRef<l_t> Lattice,
                    DataFlowAnalysisType DfAnalysisType) noexcept
      : Instr(std::move(Inst)), Fact(std::move(DfFact)),
        LatticeElement(std::move(Lattice)), AnalysisType(DfAnalysisType) {}
};

/// \brief A default implementation of the AnalysisPrinterBase. Aggregates all
/// analysis results in a vector and prints them when the analysis is finished.
template <typename AnalysisDomainTy>
class DefaultAnalysisPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  DefaultAnalysisPrinter() noexcept = default;

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t Lattice,
                  DataFlowAnalysisType AnalysisType) override {
    AnalysisResults.emplace_back(Instr, std::move(DfFact), std::move(Lattice),
                                 AnalysisType);
  }

  void doOnFinalize(llvm::raw_ostream &OS) override {
    for (const auto &Iter : AnalysisResults) {
      OS << "At IR statement: " << NToString(Iter.Instr) << '\n';
      OS << "  Fact: " << DToString(Iter.Fact) << '\n';

      if constexpr (!std::is_same_v<l_t, BinaryDomain>) {
        OS << "  Value: " << LToString(Iter.LatticeElement) << '\n';
      }
      OS << '\n';
    }
    AnalysisResults.clear();
  }

  std::vector<Warning<AnalysisDomainTy>> AnalysisResults{};
};

} // namespace psr

#endif
