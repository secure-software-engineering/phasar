#include "phasar/DataFlow.h"            // For solveIFDSProblem()
#include "phasar/PhasarLLVM.h"          // For the HelperAnalyses
#include "phasar/PhasarLLVM/DB.h"       // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h" // For the IDELinearConstantAnalysis

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-ifds-analysis-simple <LLVM-IR file>\n";
    return 1;
  }

  // Similar to IFDS, you can also use the HelperAnalyses class to reduce some
  // boilerplate code:

  using namespace std::string_literals;
  std::vector EntryPoints = {"main"s};
  // Instead of creating all the helper analyses ourselves, we can just use the
  // HelperAnalyses class. It will create the necessary information on-demand.
  //
  // You can customize the underlying algorithms by passing a
  // HelperAnalysisConfig as third parameter
  psr::HelperAnalyses HA(Argv[1], EntryPoints);
  if (!HA.getProjectIRDB()) {
    return 1;
  }

  // Here, we instantiate the linear constant analysis problem.
  // In contrast to the example in simple.cpp, we only need to pass the
  // HelperAnalyses and the entry points (this may vary, depending on the
  // analysis problem to solve)
  auto LCAProblem = psr::createAnalysisProblem<psr::IDELinearConstantAnalysis>(
      HA, EntryPoints);

  // Solving the LCAProblem. This may take some time, depending on the size of
  // the ICFG
  auto Results = psr::solveIDEProblem(LCAProblem, HA.getICFG());

  // After we have solved the LCAProblem, we can now inspect the detected
  // constants:

  const auto *MainF = HA.getProjectIRDB().getFunctionDefinition("main");
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
