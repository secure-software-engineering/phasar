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
  DefaultAnalysisPrinter(llvm::raw_ostream &OS = llvm::outs()) : OS(OS) {}

  void onResult(Warning<AnalysisDomainTy> Warn) override {
    AnalysisResults.Warn.emplace_back(std::move(Warn));
  }

  void onInitialize() override{};
  void onFinalize() const override {
    for (const auto &Iter : AnalysisResults.Warn) {

      OS << "\nAt IR statement: " << NToString(Iter.Instr) << "\n";

      OS << "\tFact: " << DToString(Iter.Fact) << "\n";

      if constexpr (std::is_same_v<l_t, BinaryDomain>) {
        OS << "Value: " << LToString(Iter.LatticeElement) << "\n";
      }
    }
  }

private:
  DataflowAnalysisResults<AnalysisDomainTy> AnalysisResults{};
  llvm::raw_ostream &OS = llvm::outs();
};

} // namespace psr

#endif
