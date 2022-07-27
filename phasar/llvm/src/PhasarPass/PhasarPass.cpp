/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarPass/Options.h"
#include "phasar/PhasarPass/PhasarPass.h"
#include "phasar/Utils/EnumFlags.h"

namespace psr {

PhasarPass::PhasarPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPass::getPassName() const { return "PhasarPass"; }

bool PhasarPass::runOnModule(llvm::Module &M) {
  // set up the IRDB
  ProjectIRDB DB({&M}, IRDBOptions::WPA);
  std::set<std::string> EntryPointsSet;
  // check if the requested entry points exist
  for (const std::string &EP : EntryPoints) {
    if (!DB.getFunctionDefinition(EP)) {
      llvm::report_fatal_error(
          ("psr error: entry point does not exist '" + EP + "'").c_str());
    }
    EntryPointsSet.insert(EP);
  }
  // set up the call-graph algorithm to be used
  CallGraphAnalysisType CGTy = toCallGraphAnalysisType(CallGraphAnalysis);
  LLVMTypeHierarchy H(DB);
  LLVMPointsToSet PT(DB);
  LLVMBasedCFG CFG;
  LLVMBasedICFG I(DB, CGTy, EntryPointsSet, &H, &PT);
  if (DataFlowAnalysis == "ifds-solvertest") {
    IFDSSolverTest IFDSTest(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver LLVMIFDSTestSolver(IFDSTest);
    LLVMIFDSTestSolver.solve();
    if (DumpResults) {
      LLVMIFDSTestSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-solvertest") {
    IDESolverTest IDETest(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver LLVMIDETestSolver(IDETest);
    LLVMIDETestSolver.solve();
    if (DumpResults) {
      LLVMIDETestSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "intra-mono-solvertest") {
    IntraMonoSolverTest Intra(&DB, &H, &I, &PT, EntryPointsSet);
    IntraMonoSolver Solver(Intra);
    Solver.solve();
    if (DumpResults) {
      Solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "inter-mono-solvertest") {
    InterMonoSolverTest Inter(&DB, &H, &I, &PT, EntryPointsSet);
    InterMonoSolver_P<InterMonoSolverTest, 3> Solver(Inter);
    Solver.solve();
    if (DumpResults) {
      Solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-const") {
    IFDSConstAnalysis ConstProblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver LLVMConstSolver(ConstProblem);
    LLVMConstSolver.solve();
    if (DumpResults) {
      LLVMConstSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-lca") {
    IFDSLinearConstantAnalysis LcaProblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver LLVMLcaSolver(LcaProblem);
    LLVMLcaSolver.solve();
    if (DumpResults) {
      LLVMLcaSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-taint") {
    TaintConfig Config(DB);
    IFDSTaintAnalysis TaintAnalysisProblem(&DB, &H, &I, &PT, Config,
                                           EntryPointsSet);
    IFDSSolver LLVMTaintSolver(TaintAnalysisProblem);
    LLVMTaintSolver.solve();
    if (DumpResults) {
      LLVMTaintSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-type") {
    IFDSTypeAnalysis TypeAnalysisProblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver LLVMTypeSolver(TypeAnalysisProblem);
    LLVMTypeSolver.solve();
    if (DumpResults) {
      LLVMTypeSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-uninit") {
    IFDSUninitializedVariables UninitializedVarProblem(&DB, &H, &I, &PT,
                                                       EntryPointsSet);
    IFDSSolver LLVMUnivSolver(UninitializedVarProblem);
    LLVMUnivSolver.solve();
    if (DumpResults) {
      LLVMUnivSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-lca") {
    IDELinearConstantAnalysis LcaProblem(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver LLVMLcaSolver(LcaProblem);
    LLVMLcaSolver.solve();
    if (DumpResults) {
      LLVMLcaSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-taint") {
    IDETaintAnalysis TaintAnalysisProblem(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver LLVMTaintSolver(TaintAnalysisProblem);
    LLVMTaintSolver.solve();
    if (DumpResults) {
      LLVMTaintSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-typestate") {
    CSTDFILEIOTypeStateDescription FileIODesc;
    IDETypeStateAnalysis TypeStateProblem(&DB, &H, &I, &PT, FileIODesc,
                                          EntryPointsSet);
    IDESolver LLVMTypeStateSolver(TypeStateProblem);
    LLVMTypeStateSolver.solve();
    if (DumpResults) {
      LLVMTypeStateSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-instinteract") {
    IDEInstInteractionAnalysis InstInteraction(&DB, &H, &I, &PT,
                                               EntryPointsSet);
    IDESolver LLVMInstInteractionSolver(InstInteraction);
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
