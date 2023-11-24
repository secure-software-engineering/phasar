#ifndef PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H

#include "phasar/PhasarLLVM/Utils/DefaultAnalysisPrinter.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include <llvm/Support/raw_ostream.h>
namespace psr {

template <typename AnalysisDomainTy>
class OnTheFlyAnalysisPrinter
    : public DefaultAnalysisPrinter<AnalysisDomainTy> {
  using l_t = typename AnalysisDomainTy::l_t;

public:
  OnTheFlyAnalysisPrinter(llvm::raw_ostream *OS)
      : DefaultAnalysisPrinter<AnalysisDomainTy>(OS), OS(OS){};

  void onInitialize() override{};
  void onResult(Warning<AnalysisDomainTy> War) override {

    *OS << "\nAt IR statement: " << NToString(War.Instr) << "\n";
    *OS << "\tFact: " << DToString(War.Fact) << "\n";
    if constexpr (std::is_same_v<l_t, BinaryDomain>) {
      *OS << "Value: " << LToString(War.LatticeElement) << "\n";
    }
  }

  void onFinalize() const override{};

private:
  MaybeUniquePtr<llvm::raw_ostream> OS;
};
} // namespace psr

#endif
