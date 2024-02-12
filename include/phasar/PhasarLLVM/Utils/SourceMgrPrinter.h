#ifndef PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#define PHASAR_PHASARLLVM_UTILS_SOURCEMGRPRINTER_H
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

std::optional<unsigned> getSourceBufId(llvm::StringRef FileName,
                                       llvm::StringMap<unsigned> &FileNameIDMap,
                                       llvm::SourceMgr &SrcMgr);

template <typename AnalysisDomainTy>
class SourceMgrPrinter : public AnalysisPrinterBase<AnalysisDomainTy> {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using l_t = typename AnalysisDomainTy::l_t;

public:
  SourceMgrPrinter(
      llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
      llvm::raw_ostream &OS = llvm::errs())
      : GetPrintMessage(std::move(PrintMessage)), OS(&OS) {}

  void onInitialize() override {}
  void onFinalize() override {}
  void onResult(n_t Instr, d_t /*DfFact*/, l_t /*Lattice*/,
                DataFlowAnalysisType AnalysisType) override {
    auto BufIdOpt =
        getSourceBufId(getFilePathFromIR(Instr), FileNameIDMap, SrcMgr);
    if (BufIdOpt.has_value()) {
      std::pair<unsigned int, unsigned int> LineAndCol =
          getLineAndColFromIR(Instr);
      /// TODO: Configuration options for warning or error
      SrcMgr.PrintMessage(
          *OS,
          SrcMgr.FindLocForLineAndColumn(BufIdOpt.value(), LineAndCol.first,
                                         LineAndCol.second),
          llvm::SourceMgr::DK_Warning, GetPrintMessage(AnalysisType));
    }
  }

private:
  llvm::StringMap<unsigned> FileNameIDMap{};
  llvm::SourceMgr SrcMgr{};
  llvm::unique_function<std::string(DataFlowAnalysisType)> GetPrintMessage;
  MaybeUniquePtr<llvm::raw_ostream> OS = &llvm::errs();
};

} // namespace psr
#endif
