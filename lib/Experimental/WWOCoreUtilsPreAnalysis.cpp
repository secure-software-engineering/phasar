#include <phasar/Experimental/WWOCoreUtilsPreAnalysis.h>

void analyzeCoreUtilsUsingPreAnalysis(ProjectIRDB &&IRDB,
                                      vector<DataFlowAnalysisType> Analyses) {}

void analyzeCoreUtilsWithoutUsingPreAnalysis(
    ProjectIRDB &&IRDB, vector<DataFlowAnalysisType> Analyses) {
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
        BOOST_LOG_SEV(lg, ERROR)
            << "Entry point '" << invalidEntryPoint << "' is not valid.";
      }
      throw logic_error("invalid entry points");
    }
    if (VariablesMap["entry_points"].as<vector<string>>().size()) {
      EntryPoints = VariablesMap["entry_points"].as<vector<string>>();
    }
  }
  // here we link every llvm module into a single module containing the entire
  // IR
  BOOST_LOG_SEV(lg, INFO)
      << "link all llvm modules into a single module for WPA ...\n";
  IRDB.linkForWPA();
  IRDB.preprocessIR();
  IRDB.print();
  // Reconstruct the inter-modular class hierarchy and virtual function tables
  BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
  LLVMTypeHierarchy CH(IRDB);
  BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
  CH.printAsDot();

  // Perform whole program analysis (WPA) analysis
  cout << "WPA_MODE HAPPENING" << endl;
  LLVMBasedICFG ICFG(CH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF,
                     EntryPoints);
  if (VariablesMap.count("callgraph_plugin")) {
    // TODO write a lambda to replace the built-in callgraph with the
    // callgraph provided by the plugin
    SOL so(VariablesMap["callgraph_plugin"].as<string>());
  }
  cout << "CONSTRUCTION OF ICFG COMPLETED" << endl;
  ICFG.print();
  ICFG.printAsDot("interproc_cfg.dot");
  // CFG is only needed for intra-procedural monotone framework
  LLVMBasedCFG CFG;
  /*
   * Perform all the analysis that the user has chosen.
   */
  // only support one analysis at a time in this experiment to simplify things
  if (Analyses.size() != 1) {
    throw runtime_error(
        "only one analysis at a time supported in this experiment");
  }
  BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << Analyses.back();
  switch (Analyses.back()) {
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
        string ModuleName = getModuleFromVal(Leak.first)->getModuleIdentifier();
        BOOST_LOG_SEV(lg, INFO)
            << "At instruction: '" << llvmIRToString(Leak.first)
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
    LLVMIntraMonotoneSolver<pair<const llvm::Value *, unsigned>, LLVMBasedCFG &>
        solver(intra, true);
    solver.solve();
    break;
  }
  case DataFlowAnalysisType::MONO_Intra_SolverTest: {
    const llvm::Function *F = IRDB.getFunction(EntryPoints.front());
    IntraMonotoneSolverTest intra(CFG, F);
    LLVMIntraMonotoneSolver<const llvm::Value *, LLVMBasedCFG &> solver(intra,
                                                                        true);
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
    // vector<string> AnalysisPlugins =
    // VariablesMap["analysis_plugin"].as<vector<string>>();
    // AnalysisPluginController PluginController(AnalysisPlugins, ICFG,
    // EntryPoints);
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
