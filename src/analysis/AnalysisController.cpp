/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "AnalysisController.h"

const map<string, ExportType> StringToExportType = {{"json", ExportType::JSON}};

const map<ExportType, string> ExportTypeToString = {{ExportType::JSON, "json"}};

ostream &operator<<(ostream &os, const ExportType &E) {
  return os << ExportTypeToString.at(E);
}

AnalysisController::AnalysisController(ProjectIRDB &&IRDB,
                                       vector<DataFlowAnalysisType> Analyses,
                                       bool WPA_MODE, bool PrintEdgeRecorder,
                                       string graph_id)
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
    vector<string> invalidEntryPoints;
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
  if (WPA_MODE) {
    // here we link every llvm module into a single module containing the entire
    // IR
    BOOST_LOG_SEV(lg, INFO)
        << "link all llvm modules into a single module for WPA ...\n";
    // START_TIMER("Link to WPA Module");
    IRDB.linkForWPA();
    // STOP_TIMER("Link to WPA Module");
  }
  IRDB.preprocessIR();
  // START_TIMER("DB Start Up");
  // DBConn &db = DBConn::getInstance();
  // STOP_TIMER("DB Start Up");
  // START_TIMER("DB Store IRDB");
  // db.storeProjectIRDB("myphasarproject", IRDB);
  // STOP_TIMER("DB Store IRDB");
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
  START_TIMER("LTH Construction");
  LLVMTypeHierarchy CH(IRDB);
  STOP_TIMER("LTH Construction");
  BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
  // START_TIMER("DB Store LTH");
  // db.storeLLVMTypeHierarchy(CH,"myphasarproject");
  // STOP_TIMER("DB Store LTH");
  // CH.printAsDot();

  // Perform whole program analysis (WPA) analysis
  if (WPA_MODE) {
    cout << "WPA_MODE HAPPENING" << endl;
    START_TIMER("ICFG Construction");
    LLVMBasedICFG ICFG(CH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF,
                       EntryPoints);

    if (VariablesMap.count("callgraph_plugin")) {
      // TODO write a lambda to replace the built-in callgraph with the
      // callgraph provided by the plugin
      SOL so(VariablesMap["callgraph_plugin"].as<string>());
    }
    STOP_TIMER("ICFG Construction");
    cout << "CONSTRUCTION OF ICFG COMPLETED" << endl;
    // ICFG.print();
    // ICFG.printAsDot("interproc_cfg.dot");
    // ICFG.getWholeModulePTG().printAsDot("wmptg.dot");
    // CFG is only needed for intra-procedural monotone framework
    LLVMBasedCFG CFG;
    /*
     * Perform all the analysis that the user has chosen.
     */
    for (DataFlowAnalysisType analysis : Analyses) {
      BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis;
      START_TIMER("DFA Runtime");
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        IFDSTaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtaintsolver(
            taintanalysisproblem, false);
        cout << "IFDS Taint Analysis started!" << endl;
        llvmtaintsolver.solve();
        cout << "IFDS Taint Analysis finished!" << endl;
        // Here we can get the leaks
        //map<const llvm::Instruction *, set<const llvm::Value *>> Leaks =
        //    taintanalysisproblem.Leaks;
        //BOOST_LOG_SEV(lg, INFO) << "Found the following leaks:";
        //if (Leaks.empty()) {
        //  BOOST_LOG_SEV(lg, INFO) << "No leaks found!";
        //} else {
        //  for (auto Leak : Leaks) {
        //    string ModuleName =
        //        getModuleFromVal(Leak.first)->getModuleIdentifier();
        //    BOOST_LOG_SEV(lg, INFO) << "At instruction: '"
        //                            << llvmIRToString(Leak.first)
        //                            << "' in file: '" << ModuleName << "'";
        //    for (auto LeakValue : Leak.second) {
        //      BOOST_LOG_SEV(lg, INFO) << llvmIRToString(LeakValue);
        //    }
        //  }
        //}
        break;
      }
      case DataFlowAnalysisType::IDE_TaintAnalysis: {
        IDETaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmtaintsolver(taintanalysisproblem, true);
        llvmtaintsolver.solve();
        break;
      }
      case DataFlowAnalysisType::IFDS_TypeAnalysis: {
        IFDSTypeAnalysis typeanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtypesolver(
            typeanalysisproblem, true);
        llvmtypesolver.solve();
        break;
      }
      case DataFlowAnalysisType::IFDS_UninitializedVariables: {
        IFDSUnitializedVariables uninitializedvarproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmunivsolver(
            uninitializedvarproblem, false);
        cout << "IFDS Uninit Analysis started!" << endl;
        llvmunivsolver.solve();
        cout << "IFDS Uninit Analysis finished!" << endl;
        if (PrintEdgeRecorder) {
          llvmunivsolver.exportJson(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_ConstAnalysis: {
        IFDSConstAnalysis constproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
            constproblem, false);
        cout << "IFDS Const Analysis started!" << endl;
        llvmconstsolver.solve();
        cout << "IFDS Const Analysis finished!" << endl;
        REG_COUNTER_WITH_VALUE("Const Init Set Size",constproblem.getInitializedSize());
        constproblem.printInitilizedSet();
        START_TIMER("DFA Result Computation");
        // TODO need to consider object fields, i.e. getelementptr instructions
        // get all stack and heap alloca instructions
        std::set<const llvm::Value *> allMemoryLoc =
            IRDB.getAllocaInstructions();
        std::set<std::string> IgnoredGlobalNames = {"llvm.used",
                                                    "llvm.compiler.used",
                                                    "llvm.global_ctors",
                                                    "llvm.global_dtors",
                                                    "vtable",
                                                    "typeinfo"};
        // add global varibales to the memory location set, except the llvm
        // intrinsic global variables
        for (auto M : IRDB.getAllModules()) {
          for (auto &GV : M->globals()) {
            if (GV.hasName()) {
              string GVName = cxx_demangle(GV.getName().str());
              if (!IgnoredGlobalNames.count(
                      GVName.substr(0, GVName.find(' ')))) {
                allMemoryLoc.insert(&GV);
              }
            }
          }
        }
        BOOST_LOG_SEV(lg, DEBUG) << "-------------";
        BOOST_LOG_SEV(lg, DEBUG) << "Allocation Instructions:";
        for (auto memloc : allMemoryLoc) {
          BOOST_LOG_SEV(lg, DEBUG) << llvmIRToString(memloc);
        }
        BOOST_LOG_SEV(lg, DEBUG) << "-------------";
        BOOST_LOG_SEV(lg, DEBUG)
            << "Printing return/resume instruction + dataflow facts:";
        for (auto RR : IRDB.getRetResInstructions()) {
          std::set<const llvm::Value *> facts =
              llvmconstsolver.ifdsResultsAt(RR);
          // Empty facts means the return/resume statement is part of not
          // analyzed function - remove all allocas of that function
          if (facts.empty()) {
            const llvm::Function *F = RR->getParent()->getParent();
            for (auto mem_itr = allMemoryLoc.begin();
                 mem_itr != allMemoryLoc.end();) {
              if (auto Inst = llvm::dyn_cast<llvm::Instruction>(*mem_itr)) {
                if (Inst->getParent()->getParent() == F) {
                  mem_itr = allMemoryLoc.erase(mem_itr);
                } else {
                  ++mem_itr;
                }
              } else {
                ++mem_itr;
              }
            }
          } else {
            BOOST_LOG_SEV(lg, DEBUG) << "Instruction: " << llvmIRToString(RR);
            for (auto fact : facts) {
              if (isAllocaInstOrHeapAllocaFunction(fact) ||
                  llvm::isa<llvm::GlobalValue>(fact)) {
                BOOST_LOG_SEV(lg, DEBUG) << "   Fact: "
                                         << constproblem.DtoString(fact);
                // remove allocas that are mutable, i.e. are valid facts
                allMemoryLoc.erase(fact);
              }
            }
          }
        }
        // write immutable locations to file
        bfs::path cfp(VariablesMap["config"].as<string>());
        // reduce the config path to just the filename - no path and no
        // extension
        std::string config = cfp.filename().string();
        std::size_t extensionPos = config.find(cfp.extension().string());
        config.replace(extensionPos, cfp.extension().size(), "");
        ofstream ResultFile;
        ResultFile.open(config + "_memlocs.txt");
        // BOOST_LOG_SEV(lg, INFO) << "-------------";
        // BOOST_LOG_SEV(lg, INFO) << "Immutable Stack/Heap Memory";
        for (auto memloc : allMemoryLoc) {
          if (auto memlocInst = llvm::dyn_cast<llvm::Instruction>(memloc)) {
            ResultFile << llvmIRToString(memlocInst) << " in function "
                       << memlocInst->getParent()->getParent()->getName().str()
                       << "\n";
          } else {
            ResultFile << llvmIRToString(memloc) << '\n';
          }
        }
        ResultFile.close();
        STOP_TIMER("DFA Result Computation");
         BOOST_LOG_SEV(lg, INFO) << "-------------";
        break;
      }
      case DataFlowAnalysisType::IFDS_SolverTest: {
        IFDSSolverTest ifdstest(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            ifdstest, false);
        cout << "IFDS Solvertest started!" << endl;
        llvmifdstestsolver.solve();
        cout << "IFDS Solvertest finished!" << endl;
        break;
      }
      case DataFlowAnalysisType::IDE_SolverTest: {
        IDESolverTest idetest(ICFG, EntryPoints);
        LLVMIDESolver<const llvm::Value *, const llvm::Value *, LLVMBasedICFG &>
            llvmidetestsolver(idetest, true);
        llvmidetestsolver.solve();
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
        LLVMInterMonotoneSolver<const llvm::Value *, 3, LLVMBasedICFG &> solver(
            inter, true);
        solver.solve();
        break;
      }
      case DataFlowAnalysisType::Plugin: {
        vector<string> AnalysisPlugins =
            VariablesMap["analysis_plugin"].as<vector<string>>();
        AnalysisPluginController PluginController(AnalysisPlugins, ICFG,
                                                  EntryPoints);
        break;
      }
      case DataFlowAnalysisType::None: {
        break;
      }
      default:
        BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
        break;
      }
      STOP_TIMER("DFA Runtime");
    }
  }
  // Perform module-wise (MW) analysis
  else {
    map<const llvm::Module *, LLVMBasedICFG> MWICFGs;
    // We build all the call- and points-to graphs which can be used for
    // all of the analysis of course.
    BOOST_LOG_SEV(lg, INFO) << "Building module-wise icfgs";
    for (auto M : IRDB.getAllModules()) {
      LLVMBasedICFG ICFG(CH, IRDB, *M, WalkerStrategy::Pointer,
                         ResolveStrategy::OTF);
      // store them away for later use
      MWICFGs.insert(make_pair(M, ICFG));
    }
    BOOST_LOG_SEV(lg, INFO) << "Performing module-wise analysis";
    IDESummaries<const llvm::Instruction *, const llvm::Value *,
                 const llvm::Function *, BinaryDomain>
        Summaries;
    // Perform all the analysis that the user has chosen.
    for (DataFlowAnalysisType analysis : Analyses) {
      for (auto &entry : MWICFGs) {
        const llvm::Module *M = entry.first;
        LLVMBasedICFG &I = entry.second;
        // Only for modules which do not contain 'main'
        if (M->getFunction("main") == nullptr) {
          BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis
                                  << " on module: " << M->getModuleIdentifier();
          switch (analysis) {
          case DataFlowAnalysisType::IFDS_TaintAnalysis: {
            IFDSTaintAnalysis taintanalysisproblem(I, {});
            LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                llvmtaintsolver(taintanalysisproblem,
                                SummaryGenerationStrategy::always_all, false);
            llvmtaintsolver.summarize();
            Summaries.addSummaries(llvmtaintsolver.getSummaries());
            BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
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
            throw invalid_argument(
                "Plugin summary generation not supported yet");
            break;
          }
          case DataFlowAnalysisType::None: {
            break;
          }
          default:
            BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
            break;
          }
        }
      }
      // After every module has been analyzed the analyses results must be
      // merged and the final results must be computed
      cout << "COMBINATION\n";
      auto M = IRDB.getModuleDefiningFunction("main");
      LLVMBasedICFG I = MWICFGs.at(M);
      BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
      for (auto entry : MWICFGs) {
        if (M != entry.first) {
          I.mergeWith(entry.second);
        }
      }
      I.printAsDot("inter.dot");
      BOOST_LOG_SEV(lg, INFO) << "Combining module-wise results";
      IFDSTaintAnalysis taintproblem(I, EntryPoints);
      LLVMMWAIFDSSolver<const llvm::Value *, LLVMBasedICFG &> taintsolver(
          taintproblem, SummaryGenerationStrategy::always_all, true);
      auto tab = Summaries.getSummaries();
      cout << "external TAB" << endl;
      cout << tab << endl;
      taintsolver.setSummaries(tab);
      taintsolver.combine();
      BOOST_LOG_SEV(lg, INFO)
          << "Combining module-wise results done, computation completed!";
    }
    BOOST_LOG_SEV(lg, INFO) << "Data-flow analyses completed.";
  }
}

void AnalysisController::writeResults(string filename) {
  ofstream ofs(filename, ofstream::app);
  ofs << FinalResultsJson.dump(1);
}
