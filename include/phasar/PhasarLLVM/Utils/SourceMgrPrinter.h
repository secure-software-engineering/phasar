#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#include "phasar/PhasarLLVM/Utils/AnalysisPrinterBase.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/OnTheFlyAnalysisPrinter.h"

#include "llvm/Support/MemoryBuffer.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/Support/SourceMgr.h>

namespace psr {
template <typename AnalysisDomainTy>
class SourceMgrPrinter : public OnTheFlyAnalysisPrinter<AnalysisDomainTy> {
public:
  SourceMgrPrinter() = default;

  void onInitialize() override {}
  void onFinalize() const override{};
  void onResult(Warning<AnalysisDomainTy> Warn) override {

    SrcMgr.PrintMessage(SrcMgr.FindLocForLineAndColumn(
                            getSourceBufId(getFilePathFromIR(Warn.Fact)),
                            getLineAndColFromIR(Warn.Fact).first,
                            getLineAndColFromIR(Warn.Fact).second),
                        llvm::SourceMgr::DK_Warning, Msg);
  }

  std::optional<unsigned> getSourceBufId(llvm::StringRef FileName) {
    if (auto It = FileNameIDMap.find(FileName); It != FileNameIDMap.end()) {
      return It->second;
    }

    auto Buf = llvm::MemoryBuffer::getFile(FileName, true);
    if (!Buf) {
      FileNameIDMap.try_emplace(FileName, std::nullopt);
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
};
} // namespace psr
#endif
