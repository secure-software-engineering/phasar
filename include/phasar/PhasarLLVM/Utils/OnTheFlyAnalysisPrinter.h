#ifndef PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H

#include "phasar/PhasarLLVM/Utils/DefaultAnalysisPrinter.h"
namespace psr {

template <typename AnalysisDomainTy>
class OnTheFlyAnalysisPrinter
    : public DefaultAnalysisPrinter<AnalysisDomainTy> {
  using l_t = typename AnalysisDomainTy::l_t;

public:
  OnTheFlyAnalysisPrinter() = default;

  void onInitialize() override{};
  void onResult(Warning<AnalysisDomainTy> War) override {

    OS << "\nAt IR statement: " << NToString(War.Instr) << "\n";
    OS << "\tFact: " << DToString(War.Fact) << "\n";
    if constexpr (std::is_same_v<l_t, BinaryDomain>) {
      OS << "Value: " << LToString(War.LatticeElement) << "\n";
    }
  }

  void onFinalize() const override{};

private:
  llvm::raw_ostream &OS = llvm::outs();
};
} // namespace psr

#endif
