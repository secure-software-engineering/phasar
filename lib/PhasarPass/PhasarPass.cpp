/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSSolverTest.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
#include <phasar/PhasarPass/Options.h>
#include <phasar/PhasarPass/PhasarPass.h>
#include <phasar/Utils/EnumFlags.h>

namespace psr {

char PhasarPass::ID = 12;

PhasarPass::PhasarPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPass::getPassName() const { return "PhasarPass"; }

bool PhasarPass::runOnModule(llvm::Module &M) {
  // set up the IRDB
  ProjectIRDB DB({&M}, IRDBOptions::WPA);
  std::set<std::string> EntryPointsSet;
  // check if the requested entry points exist
  for (const std::string &EP : EntryPoints) {
    if (!DB.getFunctionDefinition(EP)) {
      llvm::report_fatal_error("psr error: entry point does not exist '" + EP +
                               "'");
    }
    EntryPointsSet.insert(EP);
  }
  // set up the call-graph algorithm to be used
  CallGraphAnalysisType CGTy = to_CallGraphAnalysisType(CallGraphAnalysis);
  LLVMTypeHierarchy H(DB);
  LLVMPointsToInfo PT(DB);
  LLVMBasedCFG CFG;
  LLVMBasedICFG I(H, DB, CGTy, EntryPointsSet);
  if (DataFlowAnalysis == "ifds-solvertest") {
    IFDSSolverTest ifdstest(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver<IFDSSolverTest::n_t, IFDSSolverTest::d_t, IFDSSolverTest::m_t,
               IFDSSolverTest::t_t, IFDSSolverTest::v_t, IFDSSolverTest::i_t>
        llvmifdstestsolver(ifdstest);
    llvmifdstestsolver.solve();
    if (DumpResults) {
      llvmifdstestsolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-solvertest") {
    IDESolverTest idetest(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver<IDESolverTest::n_t, IDESolverTest::d_t, IDESolverTest::m_t,
              IDESolverTest::t_t, IDESolverTest::v_t, IDESolverTest::l_t,
              IDESolverTest::i_t>
        llvmidetestsolver(idetest);
    llvmidetestsolver.solve();
    if (DumpResults) {
      llvmidetestsolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "intra-mono-solvertest") {
    IntraMonoSolverTest intra(&DB, &H, &I, &PT, EntryPointsSet);
    IntraMonoSolver<IntraMonoSolverTest::n_t, IntraMonoSolverTest::d_t,
                    IntraMonoSolverTest::m_t, IntraMonoSolverTest::t_t,
                    IntraMonoSolverTest::v_t, IntraMonoSolverTest::i_t>
        solver(intra);
    solver.solve();
    if (DumpResults) {
      solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "inter-mono-solvertest") {
    InterMonoSolverTest inter(&DB, &H, &I, &PT, EntryPointsSet);
    InterMonoSolver<InterMonoSolverTest::n_t, InterMonoSolverTest::d_t,
                    InterMonoSolverTest::m_t, InterMonoSolverTest::t_t,
                    InterMonoSolverTest::v_t, InterMonoSolverTest::i_t, 3>
        solver(inter);
    solver.solve();
    if (DumpResults) {
      solver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-const") {
    IFDSConstAnalysis constproblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
               IFDSConstAnalysis::m_t, IFDSConstAnalysis::t_t,
               IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
        llvmconstsolver(constproblem);
    llvmconstsolver.solve();
    if (DumpResults) {
      llvmconstsolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-lca") {
    IFDSLinearConstantAnalysis lcaproblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver<IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
               IFDSLinearConstantAnalysis::m_t, IFDSLinearConstantAnalysis::t_t,
               IFDSLinearConstantAnalysis::v_t, IFDSLinearConstantAnalysis::i_t>
        llvmlcasolver(lcaproblem);
    llvmlcasolver.solve();
    if (DumpResults) {
      llvmlcasolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-taint") {
    TaintConfiguration<const llvm::Value *> TSF;
    IFDSTaintAnalysis TaintAnalysisProblem(&DB, &H, &I, &PT, TSF,
                                           EntryPointsSet);
    IFDSSolver<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
               IFDSTaintAnalysis::m_t, IFDSTaintAnalysis::t_t,
               IFDSTaintAnalysis::v_t, IFDSTaintAnalysis::i_t>
        LLVMTaintSolver(TaintAnalysisProblem);
    LLVMTaintSolver.solve();
    if (DumpResults) {
      LLVMTaintSolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-type") {
    IFDSTypeAnalysis typeanalysisproblem(&DB, &H, &I, &PT, EntryPointsSet);
    IFDSSolver<IFDSTypeAnalysis::n_t, IFDSTypeAnalysis::d_t,
               IFDSTypeAnalysis::m_t, IFDSTypeAnalysis::t_t,
               IFDSTypeAnalysis::v_t, IFDSTypeAnalysis::i_t>
        llvmtypesolver(typeanalysisproblem);
    llvmtypesolver.solve();
    if (DumpResults) {
      llvmtypesolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ifds-uninit") {
    IFDSUninitializedVariables uninitializedvarproblem(&DB, &H, &I, &PT,
                                                       EntryPointsSet);
    IFDSSolver<IFDSUninitializedVariables::n_t, IFDSUninitializedVariables::d_t,
               IFDSUninitializedVariables::m_t, IFDSUninitializedVariables::t_t,
               IFDSUninitializedVariables::v_t, IFDSUninitializedVariables::i_t>
        llvmunivsolver(uninitializedvarproblem);
    llvmunivsolver.solve();
    if (DumpResults) {
      llvmunivsolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-lca") {
    IDELinearConstantAnalysis lcaproblem(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
              IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
              IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
              IDELinearConstantAnalysis::i_t>
        llvmlcasolver(lcaproblem);
    llvmlcasolver.solve();
    if (DumpResults) {
      llvmlcasolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-taint") {
    IDETaintAnalysis taintanalysisproblem(&DB, &H, &I, &PT, EntryPointsSet);
    IDESolver<IDETaintAnalysis::n_t, IDETaintAnalysis::d_t,
              IDETaintAnalysis::m_t, IDETaintAnalysis::t_t,
              IDETaintAnalysis::v_t, IDETaintAnalysis::l_t,
              IDETaintAnalysis::i_t>
        llvmtaintsolver(taintanalysisproblem);
    llvmtaintsolver.solve();
    if (DumpResults) {
      llvmtaintsolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "ide-typestate") {
    CSTDFILEIOTypeStateDescription fileIODesc;
    IDETypeStateAnalysis typestateproblem(&DB, &H, &I, &PT, fileIODesc,
                                          EntryPointsSet);
    IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
              IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
              IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
              IDETypeStateAnalysis::i_t>
        llvmtypestatesolver(typestateproblem);
    llvmtypestatesolver.solve();
    if (DumpResults) {
      llvmtypestatesolver.dumpResults();
    }
  } else if (DataFlowAnalysis == "intra-mono-fullconstantpropagation") {
    // todo
  } else if (DataFlowAnalysis == "inter-mono-taint") {
    // todo
  } else if (DataFlowAnalysis == "plugin") {
    // todo
  } else if (DataFlowAnalysis == "none") {
    // do nothing
  }
  return false;
}

bool PhasarPass::doInitialization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doInitialization()\n";
  initializeLogger(InitLogger);
  // check the user's parameters
  if (EntryPoints.empty()) {
    llvm::report_fatal_error("psr error: no entry points provided");
  }
  if (to_CallGraphAnalysisType(CallGraphAnalysis) ==
      CallGraphAnalysisType::Invalid) {
    llvm::report_fatal_error("psr error: call-graph analysis does not exist");
  }
  if (to_DataFlowAnalysisType(DataFlowAnalysis) == DataFlowAnalysisType::None) {
    llvm::report_fatal_error("psr error: data-flow analysis does not exist");
  }
  return false;
}

bool PhasarPass::doFinalization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doFinalization()\n";
  return false;
}

void PhasarPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

void PhasarPass::releaseMemory() {}

void PhasarPass::print(llvm::raw_ostream &O, const llvm::Module *M) const {
  O << "I am a PhasarPass Analysis Result ;-)\n";
}

static llvm::RegisterPass<PhasarPass> phasar("phasar", "PhASAR Pass",
                                             false /* Only looks at CFG */,
                                             false /* Analysis Pass */);

} // namespace psr
