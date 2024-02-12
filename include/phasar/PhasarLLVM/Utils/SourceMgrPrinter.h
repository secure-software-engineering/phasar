#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/Support/MemoryBuffer.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h> /// TODO: quoting style

namespace psr {

std::optional<unsigned> getSourceBufId(llvm::StringRef FileName);

template <typename AnalysisDomainTy>
class SourceMgrPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
public:
  SourceMgrPrinter(
      llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
      llvm::raw_ostream &OS = llvm::errs())
      : GetPrintMessage(std::move(PrintMessage)), OS(&OS) {}

  void onInitialize() override {}
  void onFinalize() override {}
  void onResult(Warning<AnalysisDomainTy> Warn) override {
    auto BufIdOpt = getSourceBufId(getFilePathFromIR(Warn.Instr));
    if (BufIdOpt.has_value()) {
      std::pair<unsigned int, unsigned int> LineAndCol =
          getLineAndColFromIR(Warn.Instr);
      /// TODO: Configuration options for warning or error
      SrcMgr.PrintMessage(
          *OS,
          SrcMgr.FindLocForLineAndColumn(BufIdOpt.value(), LineAndCol.first,
                                         LineAndCol.second),
          llvm::SourceMgr::DK_Warning, GetPrintMessage(Warn.AnalysisType));
    }
  }

private:
  llvm::SourceMgr SrcMgr{};
  llvm::unique_function<std::string(DataFlowAnalysisType)> GetPrintMessage;
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::errs();
};

} // namespace psr
#endif
