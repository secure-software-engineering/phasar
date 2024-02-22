#ifndef PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Printer.h"

#include <optional>
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

  // Constructor
  Warning(n_t Inst, d_t DfFact, l_t Lattice,
          DataFlowAnalysisType DfAnalysisType)
      : Instr(std::move(Inst)), Fact(std::move(DfFact)),
        LatticeElement(std::move(Lattice)), AnalysisType(DfAnalysisType) {}
};

template <typename AnalysisDomainTy>
class DefaultAnalysisPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  explicit DefaultAnalysisPrinter(llvm::raw_ostream &OS = llvm::outs())
      : OS(&OS) {}

  explicit DefaultAnalysisPrinter(const llvm::Twine &Filename)
      : AnalysisPrinterBase<AnalysisDomainTy>(), OS(openFileStream(Filename)){};

  ~DefaultAnalysisPrinter() override = default;

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t Lattice,
                  DataFlowAnalysisType AnalysisType) override {
    AnalysisResults.emplace_back(Instr, DfFact, Lattice, AnalysisType);
  }

  void doOnFinalize() override {
    for (const auto &Iter : AnalysisResults) {

      *OS << "\nAt IR statement: " << NToString(Iter.Instr) << "\n";

      *OS << "\tFact: " << DToString(Iter.Fact) << "\n";

      if constexpr (!std::is_same_v<l_t, BinaryDomain>) {
        *OS << "Value: " << LToString(Iter.LatticeElement) << "\n";
      }
    }
  }

  std::vector<Warning<AnalysisDomainTy>> AnalysisResults{};
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::outs();
};

} // namespace psr

#endif
