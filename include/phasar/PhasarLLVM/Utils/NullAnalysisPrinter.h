#ifndef PHASAR_PHASARLLVM_UTILS_NULLANALYSISPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_NULLANALYSISPRINTER_H

#include "phasar/PhasarLLVM/Utils/AnalysisPrinterBase.h"

namespace psr {

template <typename AnalysisDomainTy>
class NullAnalysisPrinter final : public AnalysisPrinterBase<AnalysisDomainTy> {
public:
  static NullAnalysisPrinter *getInstance() {
    static auto Instance = NullAnalysisPrinter();
    return &Instance;
  }

  void onInitialize() override{};
  void onResult(Warning<AnalysisDomainTy> /*War*/) override{};
  void onFinalize(llvm::raw_ostream & /*OS*/) const override{};

private:
  NullAnalysisPrinter() = default;
};

} // namespace psr
#endif
