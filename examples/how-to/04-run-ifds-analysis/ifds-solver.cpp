#include "phasar/DataFlow.h"               // For the IFDSSolver
#include "phasar/PhasarLLVM/DB.h"          // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h"    // For the IFDSTaintAnalysis
#include "phasar/PhasarLLVM/Pointer.h"     // For the LLVMAliasSet
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

#include <string_view>
#include <thread>

static constexpr std::string_view Spinner[] = {"⠙", "⠹", "⠸", "⠼",
                                               "⠴", "⠦", "⠇", "⠿"};

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: run-ifds-analysis-ifds-solver <LLVM-IR file>\n";
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

  // To solve the taint problem, we now create an instance of the IFDSSolver.
  // The function psr::solveIFDSProblem() uses this solver internally as well.
  // Having the solver explicitly, allows more control over the solving process:
  psr::IFDSSolver Solver(&TaintProblem, &ICFG);

  // The simple solution. You don't really need an explicit solver for this:
  // Solver.solve();

  // Have more control over the solving process:
  if (Solver.initialize()) {
    int i = 0;

    // Perform the next 10 analysis steps, while we still have some
    while (Solver.nextN(10)) {
      // Perform some intermediate task *during* the solving process.
      // We could also interrupt the solver at any time and continue later.
      llvm::outs() << "\b\b" << Spinner[i] << ' ';
      i = (i + 1) % std::size(Spinner);

      // Wait a bit, such that we have time to see the beautiful animation for
      // our tiny example target programs:
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
    }

    Solver.finalize();
    llvm::outs() << "\nSolving finished\n";
  }

  // Here, we could loop over TaintProblem.Leaks. Instead, we will now use
  // the Solver to dump the whole raw IFDS results:
  Solver.dumpResults();
}
