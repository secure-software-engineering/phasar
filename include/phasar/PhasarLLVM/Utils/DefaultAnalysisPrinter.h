#ifndef PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_DEFAULTANALYSISPRINTER_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/Utils/AnalysisPrinterBase.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Printer.h"

#include <optional>
#include <type_traits>
#include <vector>

namespace psr {

template <typename AnalysisDomainTy>
class DefaultAnalysisPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using l_t = typename AnalysisDomainTy::l_t;

public:
  ~DefaultAnalysisPrinter() override = default;
  DefaultAnalysisPrinter() = default;

  void onResult(Warning<AnalysisDomainTy> War) override {
    AnalysisResults.War.emplace_back(std::move(War));
  }

  void onInitialize() override{};
  void onFinalize(llvm::raw_ostream &OS = llvm::outs()) const override {
    for (const auto &Iter : AnalysisResults.War) {

      OS << "\nAt IR statement: " << NToString(Iter.Instr) << "\n";

      OS << "\tFact: " << DToString(Iter.Fact) << "\n";

      if constexpr (std::is_same_v<l_t, BinaryDomain>) {
        OS << "Value: " << LToString(Iter.LatticeElement) << "\n";
      }
    }
  }

private:
  Results<AnalysisDomainTy> AnalysisResults{};
};

} // namespace psr

#endif
