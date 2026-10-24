#include "phasar/DataFlow.h"               // For MonoIFDSSolver
#include "phasar/PhasarLLVM/ControlFlow.h" // For FunctionCompressor & getEntryFunctions
#include "phasar/PhasarLLVM/DataFlow.h"    // For the MonoIFDSTaintAnalysis
#include "phasar/PhasarLLVM/Pointer.h"     // For the LLVMAliasSet
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/UsedGlobals.h"

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-monoifds-analysis-manual <LLVM-IR file>\n";
    return 1;
  }

  using namespace std::string_literals;
  std::vector EntryPoints = {"main"s};

  // Load the IR
  auto IRDB = psr::LLVMProjectIRDB::loadOrExit(Argv[1]);

  // The MonoIFDSTaintAnalysis requires alias information, so create it here
  psr::LLVMAliasSet AS(&IRDB);

  // We use a type-hierarchy to build the call-graph (LLVMBasedICFG below)
  psr::DIBasedTypeHierarchy TH(IRDB);

  // Create the ICFG
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::VTA, {"main"}, &TH,
                          &AS);

  // Assign each reachable llvm::Function in the call-graph a sequential ID.
  // This is needed for SCC computation and for the solver
  auto Funs = psr::compressFunctions(ICFG.getCallGraph(),
                                     psr::getEntryFunctions(IRDB, EntryPoints));

  // Compute the call-graph SCCs.
  auto CGSCCs = computeCGSCCs(ICFG, Funs);

  // Build a dependency-graph induced by the call-graph, collapsing each SCC to
  // a single node
  auto SCCC = computeCGSCCCallers(ICFG, Funs, CGSCCs);

  // For each CGSCC, compute which global variables are (transitively) used by
  // any function in that SCC
  auto UG = psr::computeUsedGlobals(IRDB, Funs, CGSCCs, SCCC);

  // Create the taint configuration
  psr::LLVMTaintConfig TC(IRDB);
  TC.print();
  llvm::outs() << "------------------------\n";

  // More precise alias-information; techically, this is not required, but it
  // helps a lot
  psr::FilteredLLVMAliasIterator FAI(&AS);

  // Create the taint analysis problem:
  psr::monoifds::TaintAnalysis TaintProblem(&TC, &UG, &FAI);

  // To solve the taint problem, we now create an instance of the
  // MonoIFDSSolver. Passing the HelperAnalyses here, lets the solver
  // automatically grab the needed information
  psr::monoifds::MonoIFDSSolver Solver(&TaintProblem, &ICFG);

  // Supply the solver with the previously computed helper information. If we
  // don't provide this, the solver would compute them on its own once solve()
  // is called.
  Solver.setCGSCCs(&CGSCCs).setFunctionCompressor(&Funs);

  // Solves the taint problem. This may take some time.
  Solver.solve();

  // The monoifds::TaintAnalysis is set-up to use the analysis-printer (see
  // ../04-run-ifds-analysis/otf-reporter.cpp). By default, it prints the
  // detected leaks into the given llvm::raw_ostream
  TaintProblem.emitTextReport(llvm::outs());
}
