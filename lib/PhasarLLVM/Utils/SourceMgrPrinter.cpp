#include "phasar/PhasarLLVM/Utils/SourceMgrPrinter.h"

#include "phasar/Utils/IO.h"

using namespace psr;

detail::SourceMgrPrinterBase::SourceMgrPrinterBase(
    llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
    llvm::raw_ostream &OS, llvm::SourceMgr::DiagKind WKind)
    : GetPrintMessage(std::move(PrintMessage)), OS(&OS), WarningKind(WKind) {}

detail::SourceMgrPrinterBase::SourceMgrPrinterBase(
    llvm::unique_function<std::string(DataFlowAnalysisType)> &&PrintMessage,
    const llvm::Twine &OutFileName, llvm::SourceMgr::DiagKind WKind)
    : GetPrintMessage(std::move(PrintMessage)), OS(openFileStream(OutFileName)),
      WarningKind(WKind) {}
