#include "phasar/DataFlow.h"            // For solveIFDSProblem()
#include "phasar/PhasarLLVM/DB.h"       // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h" // For the IDELinearConstantAnalysis

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-ifds-analysis-simple <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  // To solve the LCAProblem, we need an ICFG.
  // Checkout the example 02-build-call-graph for details.
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"});

  // Here, we instantiate the linear constant analysis problem.
  // We need to pass all information that the analysis requires: The IRDB,
  // the ICFG, and the functions where the analysis should start. The IDE
  // solver will walk the ICFG to analyze all statements and functions that are
  // reachable from the entrypoints.
  psr::IDELinearConstantAnalysis LCAProblem(&IRDB, &ICFG, {"main"});

  // Solving the LCAProblem. This may take some time, depending on the size of
  // the ICFG
  auto Results = psr::solveIDEProblem(LCAProblem, ICFG);

  // After we have solved the LCAProblem, we can now inspect the detected
  // constants:

  const auto *MainF = IRDB.getFunctionDefinition("main");
  if (!MainF) {
    llvm::errs() << "Required function 'main' not found\n";
    return 1;
  }

  const auto *ExitOfMain = psr::getAllExitPoints(MainF).front();

  // Get the analysis results right **after** main's return statement
  const auto &AllConstantsAtMainExit = Results.resultsAt(ExitOfMain);

  llvm::outs() << "Detected constants at " << psr::llvmIRToString(ExitOfMain)
               << ":\n";
  for (const auto &[LLVMVar, ConstVal] : AllConstantsAtMainExit) {
    llvm::outs() << "  " << psr::llvmIRToString(LLVMVar) << "\n  --> ";
    if (ConstVal.isBottom()) {
      // A "bottom" value here means that the analysis does not know the value
      // at this point and that any value may be possible.

      llvm::outs() << "<not constant>\n\n";
    } else {
      llvm::outs() << ConstVal << "\n\n";
    }
  }
}
