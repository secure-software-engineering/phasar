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
      /// TODO: getLineAndColFromIR call only once
      /// TODO: Configuration options for warning or error
      SrcMgr.PrintMessage(
          *OS,
          SrcMgr.FindLocForLineAndColumn(
              BufIdOpt.value(), getLineAndColFromIR(Warn.Instr).first,
              getLineAndColFromIR(Warn.Instr).second),
          llvm::SourceMgr::DK_Warning, GetPrintMessage(Warn.AnalysisType));
    }
  }

  /// TODO: move this to cpp file and refactor the imports
  std::optional<unsigned> getSourceBufId(llvm::StringRef FileName) {
    if (auto It = FileNameIDMap.find(FileName); It != FileNameIDMap.end()) {
      return It->second;
    }

    auto Buf = llvm::MemoryBuffer::getFile(FileName, true);
    if (!Buf) {
      PHASAR_LOG_LEVEL(WARNING, "File not accessible: " << FileName);
      return std::nullopt;
    }

    auto Id = SrcMgr.AddNewSourceBuffer(std::move(Buf.get()), llvm::SMLoc{});
    FileNameIDMap.try_emplace(FileName, Id);
    return Id;
  }

private:
  llvm::SourceMgr SrcMgr{};
  llvm::StringMap<unsigned> FileNameIDMap{};
  llvm::unique_function<std::string(DataFlowAnalysisType)> GetPrintMessage;
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::errs();
};
} // namespace psr
#endif
