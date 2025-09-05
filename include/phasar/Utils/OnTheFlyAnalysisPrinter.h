#ifndef PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_ONTHEFLYANALYSISPRINTER_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

namespace psr {

/// This class implements the AnalysisPrinterBase that prints the analysis
/// results *while* the analysis is still running.
///
/// Override doOnResult() to customize, how the results are printed.
template <typename AnalysisDomainTy>
class OnTheFlyAnalysisPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  explicit OnTheFlyAnalysisPrinter(llvm::raw_ostream &OS = llvm::outs())
      : OS(&OS) {};

  explicit OnTheFlyAnalysisPrinter(const llvm::Twine &Filename)
      : OS(openFileStream(Filename)) {};

  [[nodiscard]] bool isValid() const noexcept { return OS != nullptr; }

private:
  void doOnResult(n_t Instr, d_t DfFact, l_t LatticeElement,
                  DataFlowAnalysisType /*AnalysisType*/) override {
    assert(isValid());
    *OS << "At IR statement: " << NToString(Instr) << '\n';
    *OS << "  Fact: " << DToString(DfFact) << '\n';

    if constexpr (!std::is_same_v<l_t, BinaryDomain>) {
      *OS << "  Value: " << LToString(LatticeElement) << '\n';
    }
    *OS << '\n';
  }

  MaybeUniquePtr<llvm::raw_ostream> OS = nullptr;
};
} // namespace psr

#endif
