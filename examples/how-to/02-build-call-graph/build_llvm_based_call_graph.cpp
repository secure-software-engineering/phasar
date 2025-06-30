#include "phasar/PhasarLLVM/ControlFlow.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/DB.h"
#include "phasar/PhasarLLVM/TypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils.h"

#include "llvm/Demangle/Demangle.h"

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: build-llvm-based-call-graph <LLVM-IR file>\n";
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

  // Needed to resolve indirect calls to C++ virtual functions
  psr::LLVMVFTableProvider VTP(IRDB);

  // The resolver defines, how the call-graph construction algorithm resolves
  // indirect calls.
  // This comprises calls through a function pointer, as well as virtual
  // functions. Here, we select the Rapid Type Analysis that requires a
  // type-hierarchy as input.
  //
  // You can also write your own resolver by creating a class that inherits from
  // the psr::Resolver interface.
  psr::RTAResolver Resolver(&IRDB, &VTP, &TH);

  // You must specify at least one function as entry-point. The
  // LLVMBasedICFG will only consider those functions for the call-graph
  // that are reachable from at least on eof the entry-points.
  auto CG = psr::buildLLVMBasedCallGraph(IRDB, Resolver, {"main"});

  // Iterate over all call-sites:
  for (const auto *Call : CG.getAllVertexCallSites()) {
    if (Call->isDebugOrPseudoInst()) {
      // We may with to skip the auto-generated debug-intrinsics
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

  // You can create an LLVMBasedICFG from an already existing call-graph:
  psr::LLVMBasedICFG ICFG(std::move(CG), &IRDB);

  llvm::outs() << "--------------------------\n";

  // You can export the call-graph as dot, such that you can display it
  // using a graphviz viewer:
  ICFG.print();
}
