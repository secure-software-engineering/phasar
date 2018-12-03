/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <fstream>
#include <iostream>

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>

#include <phasar/Controller/AnalysisController.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDESummaries.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMIntraMonoSolver.h>
#include <phasar/PhasarLLVM/Plugins/AnalysisPluginController.h>
#include <phasar/PhasarLLVM/Plugins/PluginFactories.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>

using namespace std;
using namespace psr;

namespace psr {

const std::map<std::string, ExportType> StringToExportType = {
    {"json", ExportType::JSON}};

const std::map<ExportType, std::string> ExportTypeToString = {
    {ExportType::JSON, "json"}};

std::ostream &operator<<(std::ostream &os, const ExportType &E) {
  return os << ExportTypeToString.at(E);
}

AnalysisController::AnalysisController(
    ProjectIRDB &&IRDB, std::vector<DataFlowAnalysisType> Analyses,
    bool WPA_MODE, bool PrintEdgeRecorder, std::string graph_id)
    : FinalResultsJson() {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Constructed the analysis controller.");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Found the following IR files for this project: ");
  for (auto file : IRDB.getAllSourceFiles()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "\t" << file);
  }
  // Check if the chosen entry points are valid
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Check for chosen entry points.");
  vector<string> EntryPoints = {"main"};
  if (VariablesMap.count("entry-points")) {
    std::vector<std::string> invalidEntryPoints;
    for (auto &entryPoint : VariablesMap["entry-points"].as<vector<string>>()) {
      if (IRDB.getFunction(entryPoint) == nullptr) {
        invalidEntryPoints.push_back(entryPoint);
      }
    }
    if (invalidEntryPoints.size()) {
      for (auto &invalidEntryPoint : invalidEntryPoints) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, ERROR)
                      << "Entry point '" << invalidEntryPoint
                      << "' is not valid.");
      }
      throw logic_error("invalid entry points");
    }
    if (VariablesMap["entry-points"].as<vector<string>>().size()) {
      EntryPoints = VariablesMap["entry-points"].as<vector<string>>();
    }
  }
  if (WPA_MODE) {
    // here we link every llvm module into a single module containing the entire
    // IR
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg, INFO)
        << "link all llvm modules into a single module for WPA ...\n");
    START_TIMER("Link to WPA Module", PAMM_SEVERITY_LEVEL::Full);
    IRDB.linkForWPA();
    STOP_TIMER("Link to WPA Module", PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg, INFO)
        << "link all llvm modules into a single module for WPA ended\n");
  }
  IRDB.preprocessIR();

  // START_TIMER("DB Start Up", PAMM_SEVERITY_LEVEL::Full);
  // DBConn &db = DBConn::getInstance();
  // STOP_TIMER("DB Start Up", PAMM_SEVERITY_LEVEL::Full);
  // START_TIMER("DB Store IRDB", PAMM_SEVERITY_LEVEL::Full);
  // db.storeProjectIRDB("myphasarproject", IRDB);
  // STOP_TIMER("DB Store IRDB", PAMM_SEVERITY_LEVEL::Full);
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.");
  START_TIMER("CH Construction", PAMM_SEVERITY_LEVEL::Core);
  LLVMTypeHierarchy CH(IRDB);
  STOP_TIMER("CH Construction", PAMM_SEVERITY_LEVEL::Core);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Reconstruction of class hierarchy completed.");

  // llvm::errs() << "Allocated types\n";
  // for (auto alloc : IRDB.getAllocatedTypes()) {
  //   llvm::errs() << "\n";
  //   alloc->print(llvm::errs());
  //   llvm::errs() << "\n";
  // }
  // llvm::errs() << "End Allocated types\n";

  // START_TIMER("DB Store CH", PAMM_SEVERITY_LEVEL::Full);
  // db.storeLLVMTypeHierarchy(CH,"myphasarproject");
  // STOP_TIMER("DB Store CH", PAMM_SEVERITY_LEVEL::Full);
  // CH.printAsDot();
  // FinalResultsJson += CH.getAsJson();
  // START_TIMER("print all-module", PAMM_SEVERITY_LEVEL::Full);
  // ofstream ofs_module("module.log");
  // llvm::raw_os_ostream stream_module(ofs_module);
  // IRDB.getWPAModule()->print(stream_module, nullptr);
  // STOP_TIMER("print all-module", PAMM_SEVERITY_LEVEL::Full);
  //
  // auto CHJson = CH.getAsJson();
  // ofstream ofs_ch("class_hierarchy.json");
  //
  // ofs_ch << CHJson.dump();
  // WARNING
  // if (VariablesMap.count("classhierarchy_analysis")) {
  //   CH.print();
  //   CH.printAsDot("ch.dot");
  // }

  // Call graph construction stategy
  CallGraphAnalysisType CGType(
      (VariablesMap.count("callgraph-analysis"))
          ? StringToCallGraphAnalysisType.at(
                VariablesMap["callgraph-analysis"].as<string>())
          : CallGraphAnalysisType::OTF);
  // Perform whole program analysis (WPA) analysis
  if (WPA_MODE) {
    START_TIMER("CG Construction", PAMM_SEVERITY_LEVEL::Core);
    LLVMBasedICFG ICFG(CH, IRDB, CGType, EntryPoints);

    if (VariablesMap.count("callgraph-plugin")) {
      throw runtime_error("callgraph plugin not found");
    }
    STOP_TIMER("CG Construction", PAMM_SEVERITY_LEVEL::Core);
    // ICFG.printAsDot("call_graph.dot");
    // Add the ICFG to final results

    // FinalResultsJson += ICFG.getAsJson();
    // if (VariablesMap.count("callgraph-analysis")) {
    //   ICFG.print();
    //   ICFG.printAsDot("icfg.dot");
    // }
    // FinalResultsJson += ICFG.getWholeModulePTG().getAsJson();
    // if (VariablesMap.count("pointer-analysis")) {
    //   ICFG.getWholeModulePTG().print();
    //   ICFG.getWholeModulePTG().printAsDot("wptg.dot");
    // }
    // CFG is only needed for intra-procedural monotone framework
    LLVMBasedCFG CFG;
    START_TIMER("DFA Runtime", PAMM_SEVERITY_LEVEL::Core);
    /*
     * Perform all the analysis that the user has chosen.
     */
    for (DataFlowAnalysisType analysis : Analyses) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Performing analysis: " << analysis);
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        TaintSensitiveFunctions TSF;
        IFDSTaintAnalysis TaintAnalysisProblem(ICFG, TSF, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> LLVMTaintSolver(
            TaintAnalysisProblem, false);
        cout << "IFDS Taint Analysis ..." << endl;
        LLVMTaintSolver.solve();
        cout << "IFDS Taint Analysis ended" << endl;
        // FinalResultsJson += LLVMTaintSolver.getAsJson();
        if (PrintEdgeRecorder) {
          LLVMTaintSolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IDE_TaintAnalysis: {
        IDETaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmtaintsolver(taintanalysisproblem, true);
        llvmtaintsolver.solve();
        FinalResultsJson += llvmtaintsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmtaintsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IDE_TypeStateAnalysis: {
        IDETypeStateAnalysis typestateproblem(ICFG, "struct._IO_FILE",
                                              EntryPoints);
        LLVMIDESolver<const llvm::Value *, State, LLVMBasedICFG &>
            llvmtypestatesolver(typestateproblem, true);
        llvmtypestatesolver.solve();
        FinalResultsJson += llvmtypestatesolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmtypestatesolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_TypeAnalysis: {
        IFDSTypeAnalysis typeanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtypesolver(
            typeanalysisproblem, true);
        llvmtypesolver.solve();
        FinalResultsJson += llvmtypesolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmtypesolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_UninitializedVariables: {
        IFDSUnitializedVariables uninitializedvarproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmunivsolver(
            uninitializedvarproblem, false);
        cout << "IFDS UninitVar Analysis ..." << endl;
        llvmunivsolver.solve();
        cout << "IFDS UninitVar Analysis ended" << endl;
        // FinalResultsJson += llvmunivsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmunivsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_LinearConstantAnalysis: {
        IFDSLinearConstantAnalysis lcaproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<LCAPair, LLVMBasedICFG &> llvmlcasolver(lcaproblem,
                                                               true);
        llvmlcasolver.solve();
        FinalResultsJson += llvmlcasolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmlcasolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IDE_LinearConstantAnalysis: {
        IDELinearConstantAnalysis lcaproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &>
            llvmlcasolver(lcaproblem, true);
        llvmlcasolver.solve();
        FinalResultsJson += llvmlcasolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmlcasolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_ConstAnalysis: {
        IFDSConstAnalysis constproblem(ICFG, IRDB.getAllMemoryLocations(),
                                       EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
            constproblem, true);
        llvmconstsolver.solve();
        FinalResultsJson += llvmconstsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmconstsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_SolverTest: {
        IFDSSolverTest ifdstest(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            ifdstest, false);
        cout << "IFDS Solvertest ..." << endl;
        llvmifdstestsolver.solve();
        cout << "IFDS Solvertest ended" << endl;
        // FinalResultsJson += llvmifdstestsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmifdstestsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IDE_SolverTest: {
        IDESolverTest idetest(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmidetestsolver(idetest, true);
        llvmidetestsolver.solve();
        FinalResultsJson += llvmidetestsolver.getAsJson();
        if (PrintEdgeRecorder) {
          llvmidetestsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::Intra_Mono_FullConstantPropagation: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        IntraMonoFullConstantPropagation intra(CFG, F);
        LLVMIntraMonoSolver<pair<const llvm::Value *, unsigned>, LLVMBasedCFG &>
            solver(intra, true);
        solver.solve();
        break;
      }
      case DataFlowAnalysisType::Intra_Mono_SolverTest: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        IntraMonoSolverTest intra(CFG, F);
        LLVMIntraMonoSolver<const llvm::Value *, LLVMBasedCFG &> solver(intra,
                                                                        true);
        solver.solve();
        break;
      }
      case DataFlowAnalysisType::Inter_Mono_SolverTest: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        InterMonoSolverTest inter(ICFG, EntryPoints);
        CallString<typename InterMonoSolverTest::Node_t,
                   typename InterMonoSolverTest::Domain_t, 3>
            Context(&inter, &inter);
        auto solver = make_LLVMBasedIMS(inter, Context, F, true);
        solver->solve();
        break;
      }
      case DataFlowAnalysisType::Inter_Mono_TaintAnalysis: {
        const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
        InterMonoTaintAnalysis inter(ICFG, EntryPoints);
        CallString<typename InterMonoTaintAnalysis::Node_t,
                   typename InterMonoTaintAnalysis::Domain_t, 10>
            Context(&inter, &inter);
        auto solver = make_LLVMBasedIMS(inter, Context, F, true);
        solver->solve();
        solver->dumpResults();
        break;
      }
      case DataFlowAnalysisType::Plugin: {
        vector<string> AnalysisPlugins =
            VariablesMap["analysis-plugin"].as<vector<string>>();
#ifdef PHASAR_PLUGINS_ENABLED
        AnalysisPluginController PluginController(
            AnalysisPlugins, ICFG, EntryPoints, FinalResultsJson);
#endif
        break;
      }
      case DataFlowAnalysisType::None: {
        break;
      }
      default:
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, CRITICAL)
                      << "The analysis it not valid");
        break;
      }
    }
    STOP_TIMER("DFA Runtime", PAMM_SEVERITY_LEVEL::Core);
  }
  // Perform module-wise (MW) analysis
  else {
    throw runtime_error(
        "This code will follow soon with an accompanying paper!");
  }
}

void AnalysisController::writeResults(std::string filename) {
  std::ofstream ofs(filename);
  ofs << FinalResultsJson.dump(1);
}

} // namespace psr
