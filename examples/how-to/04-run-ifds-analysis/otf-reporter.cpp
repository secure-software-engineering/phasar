#include "phasar/DataFlow.h" // For solveIFDSProblem()
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/PhasarLLVM/DB.h"          // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h"    // For the IFDSTaintAnalysis
#include "phasar/PhasarLLVM/Pointer.h"     // For the LLVMAliasSet
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

namespace {
/// A listener that gets notified, whenever the taint analysis detects a leak
class LeakReporter
    : public psr::AnalysisPrinterBase<psr::LLVMIFDSAnalysisDomainDefault> {

  /// This function will be called once for each detected leak, **while the
  /// analysis is still running**.
  void doOnResult(const llvm::Instruction *LeakInst,
                  const llvm::Value *LeakFact,
                  psr::BinaryDomain /*LatticeElement*/,
                  psr::DataFlowAnalysisType /*AnalysisType*/) override {
    llvm::outs() << "Detected taint leak at " << psr::llvmIRToString(LeakInst)
                 << '\n';
    llvm::outs() << ">  leaking fact " << psr::llvmIRToShortString(LeakFact)
                 << "\n\n";
  }
};
} // namespace

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-ifds-analysis-otf-reporter <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  // The IFDSTaintAnalysis requires alias information, so create it here
  psr::LLVMAliasSet AS(&IRDB);

  // Create the taint configuration
  psr::LLVMTaintConfig TC(IRDB);
  TC.print();
  llvm::outs() << "------------------------\n";

  // Create the taint analysis problem
  psr::IFDSTaintAnalysis TaintProblem(&IRDB, &AS, &TC, {"main"},
                                      /*TaintMainArgs=*/false);

  // Create the ICFG
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                          nullptr, &AS);

  // We want to get notified, whenever the taint analysis detects a leak:
  LeakReporter Reporter;
  TaintProblem.setAnalysisPrinter(&Reporter);

  // Solving the TaintProblem. This may take some time, depending on the size of
  // the ICFG
  psr::solveIFDSProblem(TaintProblem, ICFG);

  // Don't need to loop over the leaks anymore. We have already intercepted all
  // incoming leaks with our Reporter
}
