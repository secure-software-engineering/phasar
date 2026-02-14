#include "phasar/DataFlow.h"               // For solveIFDSProblem()
#include "phasar/PhasarLLVM/DB.h"          // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h"    // For the IFDSTaintAnalysis
#include "phasar/PhasarLLVM/Pointer.h"     // For the LLVMAliasSet
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

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

  // The IFDSTaintAnalysis requires alias information, so create it here
  psr::LLVMAliasSet AS(&IRDB);

  // To tell the IFDSTaintAnalysis, which functions are actually sources, sinks
  // and sanitizers, we use the LLVMTaintConfig class.
  //
  // There are several ways of getting a taint configuration into this class:
  // - Loading a JSON file
  // - Specifying call-backs
  // - Annotating the target code
  //
  // For simplicity, we selected the annotated target code here (checkout the
  // taint.cpp target program in llvm-hello-world/target)
  psr::LLVMTaintConfig TC(IRDB);
  TC.print();
  llvm::outs() << "------------------------\n";

  // Here, we instantiate the taint analysis problem.
  // We need to pass all information that the taint analysis requires: The IRDB,
  // alias info, taint config, and the functions where the analysis should
  // start. The IFDS solver will walk the inter-procedural control-flow graph
  // (ICFG) to analyze all statements and functions that are reachable from the
  // entrypoints.
  psr::IFDSTaintAnalysis TaintProblem(&IRDB, &AS, &TC, {"main"},
                                      /*TaintMainArgs=*/false);

  // To solve the TaintProblem, we need an ICFG.
  // Checkout the example 02-build-call-graph for details.
  // Here, we select the OTF call-graph algorithm, which uses alias information
  // for indirect call resolution.
  //
  // Since we already have computed alias information, it would be wasteful to
  // let the LLVMBasedICFG compute the alias info again, so we pass the AS here.
  // The OTF analysis does not require a type-hierarchy.
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                          nullptr, &AS);

  // Solving the TaintProblem. This may take some time, depending on the size of
  // the ICFG
  // Note: solveIFDSProblem() returns the raw SolverResults, but we don't use
  // them here...
  psr::solveIFDSProblem(TaintProblem, ICFG);

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
