/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/Controller/AnalysisController.h>

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
  PAMM_FACTORY;
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Constructed the analysis controller.";
  BOOST_LOG_SEV(lg, INFO) << "Found the following IR files for this project: ";
  for (auto file : IRDB.getAllSourceFiles()) {
    BOOST_LOG_SEV(lg, INFO) << "\t" << file;
  }
  // Check if the chosen entry points are valid
  BOOST_LOG_SEV(lg, INFO) << "Check for chosen entry points.";
  vector<string> EntryPoints = {"main"};
  if (VariablesMap.count("entry_points")) {
    std::vector<std::string> invalidEntryPoints;
    for (auto &entryPoint : VariablesMap["entry_points"].as<vector<string>>()) {
      if (IRDB.getFunction(entryPoint) == nullptr) {
        invalidEntryPoints.push_back(entryPoint);
      }
    }
    if (invalidEntryPoints.size()) {
      for (auto &invalidEntryPoint : invalidEntryPoints) {
        BOOST_LOG_SEV(lg, ERROR) << "Entry point '" << invalidEntryPoint
                                 << "' is not valid.";
      }
      throw logic_error("invalid entry points");
    }
    if (VariablesMap["entry_points"].as<vector<string>>().size()) {
      EntryPoints = VariablesMap["entry_points"].as<vector<string>>();
    }
  }
  START_TIMER("IRP_runtime");
  if (WPA_MODE) {
    // here we link every llvm module into a single module containing the entire
    // IR
    BOOST_LOG_SEV(lg, INFO)
        << "link all llvm modules into a single module for WPA ...\n";
    START_TIMER("IRP_WPALink");
    IRDB.linkForWPA();
    STOP_TIMER("IRP_WPALink");
  }
  IRDB.preprocessIR();
  STOP_TIMER("IRP_runtime");
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
  START_TIMER("LTH_runtime");
  LLVMTypeHierarchy CH(IRDB);
  STOP_TIMER("LTH_runtime");
  BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
  FinalResultsJson += CH.getAsJson();
  if (VariablesMap.count("classhierarchy_analysis")) {
    CH.print();
    CH.printAsDot("ch.dot");
  }
  // Call graph construction stategy
  CallGraphAnalysisType CGType((VariablesMap.count("callgraph_analysis")) ? StringToCallGraphAnalysisType.at(
      VariablesMap["callgraph_analysis"].as<string>()) : CallGraphAnalysisType::OTF);
  WalkerStrategy CGWalker;
  ResolveStrategy CGResolve;
  switch (CGType) {
    case CallGraphAnalysisType::CHA:
      CGWalker = WalkerStrategy::Simple;
      CGResolve = ResolveStrategy::CHA;
      break;
    case CallGraphAnalysisType::RTA:
      CGWalker = WalkerStrategy::Simple;
      CGResolve = ResolveStrategy::RTA;
      break;
    case CallGraphAnalysisType::DTA:
      CGWalker = WalkerStrategy::DeclaredType;
      CGResolve = ResolveStrategy::TA;
      break;
    case CallGraphAnalysisType::VTA:
      CGWalker = WalkerStrategy::VariableType;
      CGResolve = ResolveStrategy::TA;
      break;
    case CallGraphAnalysisType::OTF:
      CGWalker = WalkerStrategy::Pointer;
      CGResolve = ResolveStrategy::OTF;
      break;
  }
  // Perform whole program analysis (WPA) analysis
  if (WPA_MODE) {
    START_TIMER("ICFG_runtime");
    LLVMBasedICFG ICFG(CH, IRDB, CGWalker, CGResolve, EntryPoints);

    if (VariablesMap.count("callgraph_plugin")) {
      // TODO write a lambda to replace the built-in callgraph with the
      // callgraph provided by the plugin
      SOL so(VariablesMap["callgraph_plugin"].as<string>());
    }
    STOP_TIMER("ICFG_runtime");
    ICFG.printAsDot("call_graph.dot");
    // Add the ICFG to final results
    FinalResultsJson += ICFG.getAsJson();
    if (VariablesMap.count("callgraph_analysis")) {
      ICFG.print();
      ICFG.printAsDot("icfg.dot");
    }
    FinalResultsJson += ICFG.getWholeModulePTG().getAsJson();
    if (VariablesMap.count("pointer_analysis")) {
      ICFG.getWholeModulePTG().print();
      ICFG.getWholeModulePTG().printAsDot("wptg.dot");
    }
    // CFG is only needed for intra-procedural monotone framework
    LLVMBasedCFG CFG;
    /*
     * Perform all the analysis that the user has chosen.
     */
    for (DataFlowAnalysisType analysis : Analyses) {
      BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis;
      START_TIMER("DFA_runtime");
      switch (analysis) {
        case DataFlowAnalysisType::IFDS_TaintAnalysis: {
          IFDSTaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
          LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtaintsolver(
              taintanalysisproblem, true);
          llvmtaintsolver.solve();
          FinalResultsJson += llvmtaintsolver.getAsJson();
          // Here we can get the leaks
          map<const llvm::Instruction *, set<const llvm::Value *>> Leaks =
              taintanalysisproblem.Leaks;
          BOOST_LOG_SEV(lg, INFO) << "Found the following leaks:";
          if (Leaks.empty()) {
            BOOST_LOG_SEV(lg, INFO) << "No leaks found!";
          } else {
            for (auto Leak : Leaks) {
              string ModuleName =
                  getModuleFromVal(Leak.first)->getModuleIdentifier();
              BOOST_LOG_SEV(lg, INFO) << "At instruction: '"
                                      << llvmIRToString(Leak.first)
                                      << "' in file: '" << ModuleName << "'";
              for (auto LeakValue : Leak.second) {
                BOOST_LOG_SEV(lg, INFO) << llvmIRToString(LeakValue);
              }
            }
          }
          break;
        }
        case DataFlowAnalysisType::IDE_TaintAnalysis: {
          IDETaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
          LLVMIDESolver<const llvm::Value *, const llvm::Value *,
                        LLVMBasedICFG &>
              llvmtaintsolver(taintanalysisproblem, true);
          llvmtaintsolver.solve();
          FinalResultsJson += llvmtaintsolver.getAsJson();
          break;
        }
        case DataFlowAnalysisType::IFDS_TypeAnalysis: {
          IFDSTypeAnalysis typeanalysisproblem(ICFG, EntryPoints);
          LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtypesolver(
              typeanalysisproblem, true);
          llvmtypesolver.solve();
          FinalResultsJson += llvmtypesolver.getAsJson();
          break;
        }
        case DataFlowAnalysisType::IFDS_UninitializedVariables: {
          IFDSUnitializedVariables uninitializedvarproblem(ICFG, EntryPoints);
          LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmunivsolver(
              uninitializedvarproblem, true);
          llvmunivsolver.solve();
          FinalResultsJson += llvmunivsolver.getAsJson();
          if (PrintEdgeRecorder) {
            llvmunivsolver.exportJSONDataModel(graph_id);
          }
          break;
        }
        case DataFlowAnalysisType::IFDS_ConstAnalysis: {
          IFDSConstAnalysis constproblem(ICFG, EntryPoints);
          LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
              constproblem, true);
          cout << "Const Analysis started!" << endl;
          llvmconstsolver.solve();
          FinalResultsJson += llvmconstsolver.getAsJson();
          cout << "Const Analysis finished!" << endl;
          // constproblem.printInitilizedSet();
          break;
        }
        case DataFlowAnalysisType::IFDS_SolverTest: {
          IFDSSolverTest ifdstest(ICFG, EntryPoints);
          LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
              llvmifdstestsolver(ifdstest, true);
          cout << "IFDS Solvertest started!" << endl;
          llvmifdstestsolver.solve();
          FinalResultsJson += llvmifdstestsolver.getAsJson();
          cout << "IFDS Solvertest finished!" << endl;
          break;
        }
        case DataFlowAnalysisType::IDE_SolverTest: {
          IDESolverTest idetest(ICFG, EntryPoints);
          LLVMIDESolver<const llvm::Value *, const llvm::Value *,
                        LLVMBasedICFG &>
              llvmidetestsolver(idetest, true);
          llvmidetestsolver.solve();
          llvmidetestsolver.getAsJson();
          break;
        }
        case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation: {
          const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
          IntraMonoFullConstantPropagation intra(CFG, F);
          LLVMIntraMonotoneSolver<pair<const llvm::Value *, unsigned>,
                                  LLVMBasedCFG &>
              solver(intra, true);
          solver.solve();
          break;
        }
        case DataFlowAnalysisType::MONO_Intra_SolverTest: {
          const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
          IntraMonotoneSolverTest intra(CFG, F);
          LLVMIntraMonotoneSolver<const llvm::Value *, LLVMBasedCFG &> solver(
              intra, true);
          solver.solve();
          break;
        }
        case DataFlowAnalysisType::MONO_Inter_SolverTest: {
          InterMonotoneSolverTest inter(ICFG, EntryPoints);
          LLVMInterMonotoneSolver<const llvm::Value *, 3, LLVMBasedICFG &>
              solver(inter, true);
          solver.solve();
          break;
        }
        case DataFlowAnalysisType::Plugin: {
          vector<string> AnalysisPlugins =
              VariablesMap["analysis_plugin"].as<vector<string>>();
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
          BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
          break;
      }
      STOP_TIMER("DFA_runtime");
    }
  }
  // Perform module-wise (MW) analysis
  else {
    map<const llvm::Module *, LLVMBasedICFG> MWICFGs;
    IDESummaries<const llvm::Instruction *, const llvm::Value *,
                 const llvm::Function *, BinaryDomain>
        Summaries;
    // We build all the call- and points-to graphs which can be used for
    // all of the analysis of course.
    BOOST_LOG_SEV(lg, INFO) << "Building module-wise icfgs";
    for (auto M : IRDB.getAllModules()) {
      BOOST_LOG_SEV(lg, INFO) << "Construct (partial) call-graph for: "
                              << M->getModuleIdentifier();
      LLVMBasedICFG ICFG(CH, IRDB, *M, CGWalker, CGResolve);
      // store them away for later use
      MWICFGs.insert(make_pair(M, ICFG));
    }
    BOOST_LOG_SEV(lg, INFO) << "Call-graphs have been constructed";
    // Perform all the analysis that the user has chosen.
    for (DataFlowAnalysisType analysis : Analyses) {
      // switch to create the module wise results (TODO the code needs
      // improvement!)
      switch (analysis) {
        case DataFlowAnalysisType::IFDS_TaintAnalysis: {
          for (auto &entry : MWICFGs) {
            const llvm::Module *M = entry.first;
            LLVMBasedICFG &I = entry.second;
            // Only for modules which do not contain 'main'
            if (M->getFunction("main") == nullptr ||
                M->getFunction("main")->isDeclaration()) {
              BOOST_LOG_SEV(lg, INFO)
                  << "Performing analysis: " << analysis
                  << " on module: " << M->getModuleIdentifier();
              IFDSTaintAnalysis taintanalysisproblem(I, {});
              LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                  llvmtaintsolver(taintanalysisproblem,
                                  SummaryGenerationStrategy::always_none,
                                  false);
              llvmtaintsolver.summarize();
              Summaries.addSummaries(llvmtaintsolver.getSummaries());
              BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
            }
          }
          break;
        }
        case DataFlowAnalysisType::IDE_TaintAnalysis: {
          throw invalid_argument("IDE summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::IFDS_TypeAnalysis: {
          throw invalid_argument("IFDS summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::IFDS_UninitializedVariables: {
          for (auto &entry : MWICFGs) {
            const llvm::Module *M = entry.first;
            LLVMBasedICFG &I = entry.second;
            // Only for modules which do not contain 'main'
            if (M->getFunction("main") == nullptr ||
                M->getFunction("main")->isDeclaration()) {
              BOOST_LOG_SEV(lg, INFO)
                  << "Performing analysis: " << analysis
                  << " on module: " << M->getModuleIdentifier();
              IFDSUnitializedVariables uninitproblem(I, {});
              LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                  llvmuninitsolver(uninitproblem,
                                   SummaryGenerationStrategy::always_none,
                                   false);
              llvmuninitsolver.summarize();
              Summaries.addSummaries(llvmuninitsolver.getSummaries());
              BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
            }
          }
          break;
        }
        case DataFlowAnalysisType::IFDS_SolverTest: {
          throw invalid_argument("IFDS summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::IDE_SolverTest: {
          throw invalid_argument("IDE summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation: {
          throw invalid_argument("Mono summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::MONO_Intra_SolverTest: {
          throw invalid_argument("Mono summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::MONO_Inter_SolverTest: {
          throw invalid_argument("Mono summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::Plugin: {
          throw invalid_argument("Plugin summary generation not supported yet");
          break;
        }
        case DataFlowAnalysisType::None: {
          // just return
          return;
          break;
        }
        default:
          BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
          break;
      }
      // switch for combining the results (TODO the code needs improvement,
      // currently just a hack!)
      // After every module has been analyzed the analyses results must be
      // merged and the final results must be computed
      switch (analysis) {
        case DataFlowAnalysisType::IFDS_TaintAnalysis: {
          auto M = IRDB.getModuleDefiningFunction("main");
          LLVMBasedICFG I = MWICFGs.at(M);
          BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
          for (auto entry : MWICFGs) {
            if (M != entry.first) {
              I.mergeWith(entry.second);
            }
          }
          I.printAsDot("call_graph.dot");
          BOOST_LOG_SEV(lg, INFO) << "Combining module-wise analysis results\n";
          IFDSTaintAnalysis problem(I, EntryPoints);
          LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &> solver(
              problem, SummaryGenerationStrategy::always_none, true);
          auto tab = Summaries.getSummaries();
          std::cout << "external TAB" << std::endl;
          std::cout << tab << std::endl;
          solver.setSummaries(tab);
          solver.combine();
          BOOST_LOG_SEV(lg, INFO)
              << "Combining module-wise results done, computation completed!";
          BOOST_LOG_SEV(lg, INFO) << "Data-flow analysis completed.";
          break;
        }
        case DataFlowAnalysisType::IFDS_UninitializedVariables: {
          auto M = IRDB.getModuleDefiningFunction("main");
          LLVMBasedICFG I = MWICFGs.at(M);
          BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
          for (auto entry : MWICFGs) {
            if (M != entry.first) {
              I.mergeWith(entry.second);
            }
          }
          I.printAsDot("call_graph.dot");
          BOOST_LOG_SEV(lg, INFO) << "Combining module-wise analysis results\n";
          IFDSUnitializedVariables problem(I, EntryPoints);
          LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &> solver(
              problem, SummaryGenerationStrategy::always_none, true);
          auto tab = Summaries.getSummaries();
          std::cout << "external TAB" << std::endl;
          std::cout << tab << std::endl;
          solver.setSummaries(tab);
          solver.combine();
          BOOST_LOG_SEV(lg, INFO)
              << "Combining module-wise results done, computation completed!";
          BOOST_LOG_SEV(lg, INFO) << "Data-flow analysis completed.";
          break;
        }
        default:
          throw invalid_argument(
              "Choosen analysis is not supported for MWA analysis, yet!");
          break;
      }
    }
  }
}

void AnalysisController::writeResults(std::string filename) {
  std::ofstream ofs(filename);
  ofs << FinalResultsJson.dump(1);
}
