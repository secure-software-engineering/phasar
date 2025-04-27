#include "phasar/DataFlow.h"               // For solveIFDSProblem()
#include "phasar/PhasarLLVM.h"             // For the HelperAnalyses
#include "phasar/PhasarLLVM/DataFlow.h"    // For the IFDSTaintAnalysis
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-ifds-analysis-simple <LLVM-IR file>\n";
    return 1;
  }

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

  // Create the taint configuration
  psr::LLVMTaintConfig TC(HA.getProjectIRDB());
  TC.print();
  llvm::outs() << "------------------------\n";

  // Create the taint analysis problem:
  // The utility function createAnalysisProblem() simplifies creating an
  // analysis problem with a HelperAnalyses object. It automatically passes the
  // right arguments
  auto TaintProblem = psr::createAnalysisProblem<psr::IFDSTaintAnalysis>(
      HA, &TC, EntryPoints, /*TaintMainArgs=*/false);

  // Solving the TaintProblem. This may take some time, depending on the size of
  // the ICFG
  psr::solveIFDSProblem(TaintProblem, HA.getICFG());

  // After we have solved the TaintProblem, we can now inspect the detected
  // leaks:
  for (const auto &[LeakInst, LeakFacts] : TaintProblem.Leaks) {
    llvm::outs() << "Detected taint leak at " << psr::llvmIRToString(LeakInst)
                 << '\n';
    for (const auto *Fact : LeakFacts) {
      llvm::outs() << ">  leaking fact " << psr::llvmIRToShortString(Fact)
                   << '\n';
    }
    llvm::outs() << '\n';
  }
}
