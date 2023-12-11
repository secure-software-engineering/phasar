#ifndef PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include <cassert>

#include <llvm/ADT/Twine.h>
#include <llvm/Support/raw_ostream.h>
namespace psr {

template <typename AnalysisDomainTy>
class OnTheFlyAnalysisPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using l_t = typename AnalysisDomainTy::l_t;

public:
  explicit OnTheFlyAnalysisPrinter(llvm::raw_ostream &OS)
      : AnalysisPrinterBase<AnalysisDomainTy>(), OS(&OS){};

  explicit OnTheFlyAnalysisPrinter<AnalysisDomainTy>(
      const llvm::Twine &Filename)
      : AnalysisPrinterBase<AnalysisDomainTy>(), OS(openFileStream(Filename)){};

  void onInitialize() override{};
  void onResult(Warning<AnalysisDomainTy> Warn) override {
    assert(isValid());
    *OS << "\nAt IR statement: " << NToString(Warn.Instr) << "\n";
    *OS << "\tFact: " << DToString(Warn.Fact) << "\n";
    if constexpr (std::is_same_v<l_t, BinaryDomain>) {
      *OS << "Value: " << LToString(Warn.LatticeElement) << "\n";
    }
  }

  void onFinalize() const override{};

  bool isValid() { return OS != nullptr; }

private:
  MaybeUniquePtr<llvm::raw_ostream> OS = nullptr;
};
} // namespace psr

#endif
