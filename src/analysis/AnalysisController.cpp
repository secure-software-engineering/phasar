#include "AnalysisController.h"

const map<string, ExportType> StringToExportType = {{"json", ExportType::JSON}};

const map<ExportType, string> ExportTypeToString = {{ExportType::JSON, "json"}};

ostream &operator<<(ostream &os, const ExportType &E) {
  return os << ExportTypeToString.at(E);
}

AnalysisController::AnalysisController(ProjectIRDB &&IRDB,
                                       vector<DataFlowAnalysisType> Analyses,
                                       bool WPA_MODE, bool Mem2Reg_MODE,
                                       bool PrintEdgeRecorder, string graph_id)
    : FinalResultsJson() {
  PAMM &p = PAMM::getInstance();
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
//    START_TIMER("WPA link time");
    IRDB.linkForWPA();
//    STOP_TIMER("WPA link time");
  }
  START_TIMER("IR preprocessing");
  IRDB.preprocessIR();
  STOP_TIMER("IR preprocessing");
//  IRDB.print();
//  DBConn::getInstance();
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
  START_TIMER("LTH construction");
  LLVMTypeHierarchy CH(IRDB);
  STOP_TIMER("LTH construction");
  BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
//  CH.printAsDot();

  // Perform whole program analysis (WPA) analysis
  if (WPA_MODE) {
    cout << "WPA_MODE HAPPENING" << endl;
    START_TIMER("ICFG construction");
    LLVMBasedICFG ICFG(CH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF,
                       EntryPoints);
    STOP_TIMER("ICFG construction");
    cout << "CONSTRUCTION OF ICFG COMPLETED" << endl;
//    ICFG.print();
//    ICFG.printAsDot("interproc_cfg.dot");
    // CFG is only needed for intra-procedural monotone framework
    LLVMBasedCFG CFG;
    /*
     * Perform all the analysis that the user has chosen.
     */
    for (DataFlowAnalysisType analysis : Analyses) {
      BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis;
      START_TIMER("Analysis runtime");
      switch (analysis) {
      case DataFlowAnalysisType::IFDS_TaintAnalysis: {
        IFDSTaintAnalysis taintanalysisproblem(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmtaintsolver(
            taintanalysisproblem, true);
        llvmtaintsolver.solve();
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
            uninitializedvarproblem, true);
        llvmunivsolver.solve();
        if (PrintEdgeRecorder) {
          llvmunivsolver.exportJSONDataModel(graph_id);
        }
        break;
      }
      case DataFlowAnalysisType::IFDS_SolverTest: {
        IFDSSolverTest ifdstest(ICFG, EntryPoints);
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            ifdstest, true);
        llvmifdstestsolver.solve();
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
        AnalysisPlugin(VariablesMap["analysis_interface"].as<string>(),
                       VariablesMap["analysis_plugin"].as<string>(), ICFG,
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
      STOP_TIMER("Analysis runtime");
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

    // Some in-place testing
    // // start: module_wise_5 test
    // auto& Mod = *IRDB.getModuleDefiningFunction("main");
    // auto& Mod_2 = *IRDB.getModuleDefiningFunction("_Z3barPi");
    // LLVMBasedICFG ICFG(CH, IRDB, Mod, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // ICFG.printAsDot("icfg_main.dot");
    // ICFG.printInternalPTGAsDot("wptg_main.dot");
    // LLVMBasedICFG ICFG_2(CH, IRDB, Mod_2, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // ICFG_2.printAsDot("icfg_bar.dot");
    // ICFG_2.printInternalPTGAsDot("wptg_bar.dot");
    // cout << "@@@@@@@@@@@@@@@@@@@@@ call graph after merge
    // @@@@@@@@@@@@@@@@@@@@@@\n";
    // ICFG.mergeWith(ICFG_2);
    // ICFG.printAsDot("icfg_after_merge.dot");
    // ICFG.printInternalPTGAsDot("wptg_after_merge.dot");
    // /// end: module_wise_5 test

    // // start: module_wise_6 test
    // auto& M_m = *IRDB.getModuleDefiningFunction("main");
    // auto& M_1 = *IRDB.getModuleDefiningFunction("_Z3fooRi");
    // auto& M_2 = *IRDB.getModuleDefiningFunction("_Z3barRi");
    // LLVMBasedICFG I(CH, IRDB, M_m, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // LLVMBasedICFG J(CH, IRDB, M_1, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // LLVMBasedICFG K(CH, IRDB, M_2, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // I.printAsDot("icfg_main.dot");
    // I.printInternalPTGAsDot("ptg_main.dot");
    // J.printAsDot("icfg_foo.dot");
    // J.printInternalPTGAsDot("ptg_foo.dot");
    // K.printAsDot("icfg_bar.dot");
    // K.printInternalPTGAsDot("ptg_bar.dot");
    // // merge once
    // I.mergeWith(J);
    // I.printAsDot("icfg_intermed.dot");
    // I.printInternalPTGAsDot("ptg_intermed.dot");
    // // merge twice
    // I.mergeWith(K);
    // I.printAsDot("icfg_final.dot");
    // I.printInternalPTGAsDot("ptg_final.dot");
    // // end: module_wise_6 test

    // // start: module_wise_7 test
    // auto P = *IRDB.getPointsToGraph("main");
    // auto Q = *IRDB.getPointsToGraph("_Z3fooRiS_S_");
    // auto R = *IRDB.getPointsToGraph("_Z3barRi");
    // P.printAsDot("pure_ptg_main.dot");
    // Q.printAsDot("pure_ptg_foo.dot");
    // R.printAsDot("pure_ptg_bar.dot");

    // int i = 0;
    // for (auto &BB : *IRDB.getFunction("main")) {
    // 	for (auto &I : BB) {
    // 		if (auto *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
    // 			++i;
    // 			if (i == 1)
    // 				P.mergeWith(Q, llvm::ImmutableCallSite(Call),
    // Call->getCalledFunction());
    // 			else if (i == 2)
    // 				P.mergeWith(R, llvm::ImmutableCallSite(Call),
    // Call->getCalledFunction());
    // 		}
    // 	}
    // }
    // P.printAsDot("pure_final_ptg.dot");
    // auto& M_m = *IRDB.getModuleDefiningFunction("main");
    // auto& M_1 = *IRDB.getModuleDefiningFunction("_Z3fooRiS_S_");
    // LLVMBasedICFG I(CH, IRDB, M_m, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // LLVMBasedICFG J(CH, IRDB, M_1, WalkerStrategy::Pointer,
    // ResolveStrategy::OTF);
    // auto P = *IRDB.getPointsToGraph("main");
    // auto Q = *IRDB.getPointsToGraph("_Z3fooRiS_S_");
    // auto R = *IRDB.getPointsToGraph("_Z3incRi");
    // Q.printAsDot("pure_ptg_foo.dot");
    // P.printAsDot("pure_ptg_main.dot");
    // R.printAsDot("pure_ptg_inc.dot");
    // I.printInternalPTGAsDot("ptg_mod_main.dot");
    // J.printInternalPTGAsDot("ptg_mod_foo.dot");
    // I.mergeWith(J);
    // I.printInternalPTGAsDot("ptg_mod_final.dot");
    // I.printAsDot("icfg_final.dot");
    // // end: module_wise_7 test

    // Perform all the analysis that the user has chosen.
    for (DataFlowAnalysisType analysis : Analyses) {
      for (auto &entry : MWICFGs) {
        IFDSSummaryPool<const llvm::Value *, const llvm::Instruction *> Pool;
        const llvm::Module *M = entry.first;
        LLVMBasedICFG &I = entry.second;
        BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis
                                << " on module: " << M->getModuleIdentifier();
        switch (analysis) {
        case DataFlowAnalysisType::IFDS_TaintAnalysis: {
          for (auto FunctionName : I.getDependencyOrderedFunctions()) {
            IFDSTaintAnalysis taintanalysisproblem(I);
            LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
                llvmtaintsolver(taintanalysisproblem, true);
            llvmtaintsolver.solve();
          }

          // IDESummaries<const llvm::Instruction *,
          //              const llvm::Value *,
          //              const llvm::Function *,
          //              const llvm::Value *> Summaries;
          // for (auto& Fname : I.getDependencyOrderedFunctions()) {
          //   BOOST_LOG_SEV(lg, INFO) << "Generate summary for: '" << Fname;
          //   IFDSTaintAnalysis taintanalysisproblem(I, {"_Z2idi"});
          //   IDESummaryGenerator<const llvm::Instruction *,
          //                       const llvm::Value *,
          //                       const llvm::Function *,
          //                       LLVMBasedICFG &,
          //                       const llvm::Value *,
          //                       IFDSTaintAnalysis, LLVMIFDSSolver<
          //                           const llvm::Value *,
          //                           LLVMBasedICFG &>>
          //                               Generator("_Z2idi", I, Summaries,
          //                               SummaryGenerationCTXStrategy::always_all);
          // }
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
          // This code is just for testing, it should be moved to the
          // LLVMIFDSSummaryGenerator/ IFDSSummaryGenerator!
          // Check and test the summary generation:
          // for (auto& Fname : I.getDependencyOrderedFunctions()) {
          //   BOOST_LOG_SEV(lg, INFO) << "Generate summary for: '" << Fname
          //                           << "'\n";
          //   LLVMIFDSSummaryGenerator<LLVMBasedICFG&,
          //   IFDSUnitializedVariables>
          //       Generator(M->getFunction(Fname), I,
          //                 SummaryGenerationCTXStrategy::all_and_none);
          // }
          // Pool.insert(Generator.generateSummaryFlowFunction());
          // BOOST_LOG_SEV(lg, INFO) << "Generated summaries!";
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
          break;
        }
        default:
          BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
          break;
        }
      }
      // After every module has been analyzed the analyses results must be
      // merged and the final results must be computed
      // auto M = IRDB.getModuleDefiningFunction("main");
      BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
      // LLVMBasedICFG I = MWICFGs.at(M);
      // for (auto& entry : MWICFGs) {
      //   if (entry.first != M) {
      //     I.mergeWith(entry.second);
      //   }
      // }

      // // TODO this is just testing!
      // IFDSUnitializedVariables uninitializedvarproblem(I, EntryPoints);
      // LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmunivsolver(
      //     uninitializedvarproblem, true);
      // llvmunivsolver.solve();

      BOOST_LOG_SEV(lg, INFO) << "Combining module-wise results";
      // TODO Start at the main function and iterate over the entire program
      // combining all results!
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
