#include "phasar/DataFlow.h"            // For the IDESolver
#include "phasar/PhasarLLVM/DB.h"       // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h" // For the IDELinearConstantAnalysis

#include <string_view>
#include <thread>

static constexpr std::string_view Spinner[] = {"⠙", "⠹", "⠸", "⠼",
                                               "⠴", "⠦", "⠇", "⠿"};

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

  // To solve the linear constant analysis problem, we now create an instance of
  // the IDESolver. The function psr::solveIDEProblem() uses this solver
  // internally as well. Having the solver explicitly, allows more control over
  // the solving process:
  psr::IDESolver Solver(&LCAProblem, &ICFG);

  // The simple solution. You don't really need an explicit solver for this:
  // Solver.solve();

  // Have more control over the solving process:
  Solver.initialize();
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

  // In contrast to the IFDSSolver, finalize may take some time with IDE.
  // It will still be significantly faster than the above loop.
  auto Results = Solver.finalize();
  llvm::outs() << "\nSolving finished\n";

  // After we have solved the LCAProblem, we can now inspect the detected
  // constants. Instead of manually looping, will now dump the whole raw IDE
  // results:
  Results.dumpResults(ICFG);
}
