#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H

#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMSourceManager.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {

namespace detail {
class SourceMgrPrinterBase {
public:
  explicit SourceMgrPrinterBase(
      llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
      llvm::raw_ostream &OS = llvm::errs(),
      llvm::SourceMgr::DiagKind WKind = llvm::SourceMgr::DK_Warning);

  explicit SourceMgrPrinterBase(
      llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
      const llvm::Twine &OutFileName,
      llvm::SourceMgr::DiagKind WKind = llvm::SourceMgr::DK_Warning);

protected:
  LLVMSourceManager SrcMgr;

  llvm::unique_function<std::string(DataFlowAnalysisType)> GetPrintMessage;
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::errs();
  llvm::SourceMgr::DiagKind WarningKind;
};
} // namespace detail

template <typename AnalysisDomainTy>
class SourceMgrPrinter : public AnalysisPrinterBase<AnalysisDomainTy>,
                         private detail::SourceMgrPrinterBase {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  explicit SourceMgrPrinter(
      llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
      llvm::raw_ostream &OS = llvm::errs(),
      llvm::SourceMgr::DiagKind WKind = llvm::SourceMgr::DK_Warning)
      : detail::SourceMgrPrinterBase(std::move(PrintMessage), OS, WKind) {}

private:
  void doOnResult(n_t Inst, d_t Fact, l_t /*Value*/,
                  DataFlowAnalysisType AnalysisType) override {
    auto SrcLoc = SrcMgr.getDebugLocation(Inst);
    if constexpr (std::is_convertible_v<d_t, const llvm::Value *>) {
      if (!SrcLoc) {
        SrcLoc =
            SrcMgr.getDebugLocation(static_cast<const llvm::Value *>(Fact));
      }
    }

    if (SrcLoc) {
      SrcMgr.print(*OS, *SrcLoc, WarningKind, GetPrintMessage(AnalysisType));
    }
  }
};

} // namespace psr
#endif
