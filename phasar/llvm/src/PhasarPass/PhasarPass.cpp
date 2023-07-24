/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarPass/PhasarPass.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/DataFlow/Mono/Solver/InterMonoSolver.h"
#include "phasar/DataFlow/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTypeAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/InterMonoSolverTest.h"
#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarPass/Options.h"
#include "phasar/Utils/EnumFlags.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

PhasarPass::PhasarPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPass::getPassName() const { return "PhasarPass"; }

bool PhasarPass::runOnModule(llvm::Module &M) {
  // set up the IRDB
  LLVMProjectIRDB DB(&M);

  // check if the requested entry points exist
  for (const std::string &EP : EntryPoints) {
    if (!DB.getFunctionDefinition(EP)) {
      llvm::report_fatal_error(
          ("psr error: entry point does not exist '" + EP + "'").c_str());
    }
  }
  // set up the call-graph algorithm to be used
  CallGraphAnalysisType CGTy = toCallGraphAnalysisType(CallGraphAnalysis);
  LLVMTypeHierarchy H(DB);
  LLVMAliasSet PT(&DB);
  LLVMBasedCFG CFG;
  LLVMBasedICFG I(&DB, CGTy, EntryPoints, &H, &PT);
  if (DataFlowAnalysis == "ifds-solvertest") {
    IFDSSolverTest IFDSTest(&DB, EntryPoints);

    IFDSSolver LLVMIFDSTestSolver(IFDSTest, &I);
    LLVMIFDSTestSolver.solve();
    if (DumpResults) {
      LLVMIFDSTestSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-solvertest") {
    IDESolverTest IDETest(&DB, EntryPoints);
    IDESolver LLVMIDETestSolver(IDETest, &I);
    LLVMIDETestSolver.solve();
    if (DumpResults) {
      LLVMIDETestSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "intra-mono-solvertest") {
    IntraMonoSolverTest Intra(&DB, &H, &I, &PT, EntryPoints);
    IntraMonoSolver Solver(Intra);
    Solver.solve();
    if (DumpResults) {
      Solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "inter-mono-solvertest") {
    InterMonoSolverTest Inter(&DB, &H, &I, &PT, EntryPoints);
    InterMonoSolver_P<InterMonoSolverTest, 3> Solver(Inter);
    Solver.solve();
    if (DumpResults) {
      Solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-const") {
    IFDSConstAnalysis ConstProblem(&DB, &PT, EntryPoints);
    IFDSSolver LLVMConstSolver(ConstProblem, &I);
    LLVMConstSolver.solve();
    if (DumpResults) {
      LLVMConstSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-taint") {
    LLVMTaintConfig Config(DB);
    IFDSTaintAnalysis TaintAnalysisProblem(&DB, &PT, &Config, EntryPoints);
    IFDSSolver LLVMTaintSolver(TaintAnalysisProblem, &I);
    LLVMTaintSolver.solve();
    if (DumpResults) {
      LLVMTaintSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-type") {
    IFDSTypeAnalysis TypeAnalysisProblem(&DB, EntryPoints);
    IFDSSolver LLVMTypeSolver(TypeAnalysisProblem, &I);
    LLVMTypeSolver.solve();
    if (DumpResults) {
      LLVMTypeSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-uninit") {
    IFDSUninitializedVariables UninitializedVarProblem(&DB, EntryPoints);
    IFDSSolver LLVMUnivSolver(UninitializedVarProblem, &I);
    LLVMUnivSolver.solve();
    if (DumpResults) {
      LLVMUnivSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-lca") {
    IDELinearConstantAnalysis LcaProblem(&DB, &I, EntryPoints);
    IDESolver LLVMLcaSolver(LcaProblem, &I);
    LLVMLcaSolver.solve();
    if (DumpResults) {
      LLVMLcaSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-typestate") {
    CSTDFILEIOTypeStateDescription FileIODesc;
    IDETypeStateAnalysis TypeStateProblem(&DB, &PT, &FileIODesc, EntryPoints);
    IDESolver LLVMTypeStateSolver(TypeStateProblem, &I);
    LLVMTypeStateSolver.solve();
    if (DumpResults) {
      LLVMTypeStateSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-instinteract") {
    IDEInstInteractionAnalysis InstInteraction(&DB, &I, &PT, EntryPoints);
    IDESolver LLVMInstInteractionSolver(InstInteraction, &I);
    LLVMInstInteractionSolver.solve();
    if (DumpResults) {
      LLVMInstInteractionSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "intra-mono-fullconstantpropagation") {
    // todo
  } else if (DataFlowAnalysis == "inter-mono-taint") {
    // todo
  } else if (DataFlowAnalysis == "none") {
    // do nothing
  }
  return false;
}

bool PhasarPass::doInitialization(llvm::Module & /*M*/) {
  llvm::outs() << "PhasarPass::doInitialization()\n";
  InitLogger ? Logger::enable() : Logger::disable();
  // check the user's parameters
  if (EntryPoints.empty()) {
    llvm::report_fatal_error("psr error: no entry points provided");
  }
  if (toCallGraphAnalysisType(CallGraphAnalysis) ==
      CallGraphAnalysisType::Invalid) {
    llvm::report_fatal_error("psr error: call-graph analysis does not exist");
  }
  if (toDataFlowAnalysisType(DataFlowAnalysis) == DataFlowAnalysisType::None) {
    llvm::report_fatal_error("psr error: data-flow analysis does not exist");
  }
  return false;
}

bool PhasarPass::doFinalization(llvm::Module & /*M*/) {
  llvm::outs() << "PhasarPass::doFinalization()\n";
  return false;
}

void PhasarPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

void PhasarPass::releaseMemory() {}

void PhasarPass::print(llvm::raw_ostream &O, const llvm::Module * /*M*/) const {
  O << "I am a PhasarPass Analysis Result ;-)\n";
}

static const llvm::RegisterPass<PhasarPass>
    Phasar("phasar", "PhASAR Pass", false /* Only looks at CFG */,
           false /* Analysis Pass */);

} // namespace psr
