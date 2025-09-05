#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H

#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMSourceManager.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {

template <typename AnalysisDomainTy>
class SourceMgrPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  explicit SourceMgrPrinter(
      llvm::unique_function<std::string(n_t Inst, ByConstRef<d_t> Fact,
                                        ByConstRef<l_t>, DataFlowAnalysisType)>
          &&GetMessage,
      llvm::raw_ostream &OS = llvm::errs(),
      llvm::SourceMgr::DiagKind WKind = llvm::SourceMgr::DK_Warning)
      : OS(&OS), WarningKind(WKind), GetPrintMessage(std::move(GetMessage)) {}

private:
  void doOnResult(n_t Inst, d_t Fact, l_t Value,
                  DataFlowAnalysisType AnalysisType) override {
    auto SrcLoc = SrcMgr.getDebugLocation(Inst);
    if constexpr (std::is_convertible_v<d_t, const llvm::Value *>) {
      if (!SrcLoc) {
        SrcLoc =
            SrcMgr.getDebugLocation(static_cast<const llvm::Value *>(Fact));
      }
    }

    auto Msg = GetPrintMessage(Inst, Fact, Value, AnalysisType);
    if (SrcLoc) {
      SrcMgr.print(*OS, *SrcLoc, WarningKind, Msg);
    } else {
      llvm::SMDiagnostic Diag("", WarningKind, Msg);
      Diag.print(nullptr, *OS);
    }
  }

  LLVMSourceManager SrcMgr;

  llvm::raw_ostream *OS = &llvm::errs();
  llvm::SourceMgr::DiagKind WarningKind;
  llvm::unique_function<std::string(n_t Inst, ByConstRef<d_t> Fact,
                                    ByConstRef<l_t>, DataFlowAnalysisType)>
      GetPrintMessage;
};

} // namespace psr
#endif
