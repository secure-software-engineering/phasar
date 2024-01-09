#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#include "phasar/PhasarLLVM/Utils/AnalysisPrinterBase.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/OnTheFlyAnalysisPrinter.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/Support/MemoryBuffer.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

namespace psr {
template <typename AnalysisDomainTy>
class SourceMgrPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
public:
  SourceMgrPrinter() = default;
  SourceMgrPrinter<AnalysisDomainTy>(llvm::raw_ostream &OS) : OS(&OS){};

  void onInitialize() override {}
  void onFinalize() const override{};
  void onResult(Warning<AnalysisDomainTy> Warn) override {
    auto BufIdOpt = getSourceBufId(getFilePathFromIR(Warn.Instr));
    if (BufIdOpt.has_value()) {
      SrcMgr.PrintMessage(*OS,
                          SrcMgr.FindLocForLineAndColumn(
                              BufIdOpt.value(),
                              getLineAndColFromIR(Warn.Instr).first,
                              getLineAndColFromIR(Warn.Instr).second),
                          llvm::SourceMgr::DK_Warning, Msg);
    }
  }

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
  llvm::SourceMgr SrcMgr;
  llvm::StringMap<unsigned> FileNameIDMap;
  static constexpr llvm::StringLiteral Msg = "Phasar found an error";
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::errs();
};
} // namespace psr
#endif
