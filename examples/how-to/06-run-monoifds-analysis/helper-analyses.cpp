#include "phasar/DataFlow.h"            // For MonoIFDSSolver
#include "phasar/PhasarLLVM.h"          // For the HelperAnalyses
#include "phasar/PhasarLLVM/DataFlow.h" // For the MonoIFDSTaintAnalysis
#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs()
        << "USAGE: run-monoifds-analysis-helper-analyses <LLVM-IR file>\n";
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

  // More precise alias-information; techically, this is not required, but it
  // helps a lot
  psr::FilteredLLVMAliasIterator FAI(HA.getAliasInfo());

  // Create the taint analysis problem:
  psr::monoifds::TaintAnalysis TaintProblem(&TC, &HA.getUsedGlobals(), &FAI);

  // To solve the taint problem, we now create an instance of the
  // MonoIFDSSolver. Passing the HelperAnalyses here, lets the solver
  // automatically grab the needed information
  psr::monoifds::MonoIFDSSolver Solver(&TaintProblem, HA);

  // Solves the taint problem. This may take some time.
  Solver.solve();

  // The monoifds::TaintAnalysis is set-up to use the analysis-printer (see
  // ../04-run-ifds-analysis/otf-reporter.cpp). By default, it prints the
  // detected leaks into the given llvm::raw_ostream
  TaintProblem.emitTextReport(llvm::outs());
}
