#include "phasar/PhasarLLVM/ControlFlow.h"
#include "phasar/PhasarLLVM/DB.h"
#include "phasar/PhasarLLVM/TypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils.h"

#include "llvm/Demangle/Demangle.h"

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: build-llvm-based-icfg <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  if (!IRDB.getFunctionDefinition("main")) {
    llvm::errs() << "Required function 'main' not found\n";
    return 1;
  }

  // We may wish to use type-information for construcing the call-graph
  psr::DIBasedTypeHierarchy TH(IRDB);

  // The easiest way of getting a call graph is by construcing an
  // inter-procedural control-flow graph (ICFG):
  //
  // You can select the call-graph resolver algorithm using the
  // CallGraphAnalysisType enum. The LLVMBasedICFG will create required
  // data-structures that you don't explicitly pass in, on demand.
  // Here, we select the Rapid Type Analysis that requires a type-hierarchy as
  // input.
  //
  // You must specify at least one function as entry-point. The LLVMBasedICFG
  // will only consider those functions for the call-graph that are
  // (transitively) reachable from at least on of the entry-points.
  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::RTA, {"main"},
                          &TH);
  const auto &CG = ICFG.getCallGraph();

  // Iterate over all call-sites:
  for (const auto *Call : CG.getAllVertexCallSites()) {
    if (Call->isDebugOrPseudoInst()) {
      // We may wish to skip the auto-generated debug-intrinsics
      continue;
    }

    llvm::outs() << "Found call-site: " << psr::llvmIRToString(Call) << '\n';

    // The probably most important function: getCalleesOfCallAt()
    for (const auto *CalleeFun : CG.getCalleesOfCallAt(Call)) {
      llvm::outs() << ">  calling "
                   << llvm::demangle(CalleeFun->getName().str()) << '\n';
    }
    llvm::outs() << '\n';
  }

  llvm::outs() << "--------------------------\n";

  // You can also go the other way around:
  for (const auto *Fun : CG.getAllVertexFunctions()) {
    llvm::outs() << "Found Function: " << llvm::demangle(Fun->getName().str())
                 << '\n';

    // The probably second-most important function: getCallersOf()
    for (const auto *CallSite : CG.getCallersOf(Fun)) {
      llvm::outs() << ">  called from " << psr::llvmIRToString(CallSite)
                   << '\n';
    }
    llvm::outs() << '\n';
  }

  llvm::outs() << "--------------------------\n";

  // You can also export the call-graph as dot, such that you can display it
  // using a graphviz viewer:
  ICFG.print();
}
