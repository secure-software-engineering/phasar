#include "AnalysisController.hh"

const map<string, DataFlowAnalysisType> DataFlowAnalysisTypeMap = { { "ifds_uninit", DataFlowAnalysisType::IFDS_UninitializedVariables },
																																	{ "ifds_taint", DataFlowAnalysisType::IFDS_TaintAnalysis },
																																	{ "ifds_type", DataFlowAnalysisType::IFDS_TypeAnalysis },
																																	{ "ide_taint", DataFlowAnalysisType::IDE_TaintAnalysis },
																																	{ "ifds_solvertest", DataFlowAnalysisType::IFDS_SolverTest },
																																	{ "ide_solvertest", DataFlowAnalysisType::IDE_SolverTest },
																																	{ "mono_intra_fullconstpropagation", DataFlowAnalysisType::MONO_Intra_FullConstantPropagation },
																																	{ "mono_intra_solvertest", DataFlowAnalysisType::MONO_Intra_SolverTest },
																																	{ "mono_inter_solvertest", DataFlowAnalysisType::MONO_Inter_SolverTest },
																																	{ "plugin", DataFlowAnalysisType::Plugin },
																																	{ "none", DataFlowAnalysisType::None } };

ostream& operator<<(ostream& os, const DataFlowAnalysisType& D) {
	static const array<string, 11> str {{
		"AnalysisType::IFDS_UninitializedVariables",
		"AnalysisType::IFDS_TaintAnalysis",
		"AnalysisType::IDE_TaintAnalysis",
		"AnalysisType::IFDS_TypeAnalysis",
		"AnalysisType::IFDS_SolverTest",
		"AnalysisType::IDE_SolverTest",
		"AnalysisType::MONO_Intra_FullConstantPropagation",
		"AnalysisType::MONO_Intra_SolverTest",
		"AnalysisType::MONO_Inter_SoverTest",
		"AnalysisType::Plugin",
		"AnalysisType::None",
	}};
	return os << str.at(static_cast<underlying_type_t<DataFlowAnalysisType>>(D));
}

const map<string, ExportType> ExportTypeMap = { { "json" , ExportType::JSON } };

ostream& operator<<(ostream& os, const ExportType& E) {
	static const array<string, 1> str {{
		"ExportType::JSON"
	}};
	return os << str.at(static_cast<underlying_type_t<ExportType>>(E));
}

AnalysisController::AnalysisController(ProjectIRCompiledDB &&IRDB,
                     vector<DataFlowAnalysisType> Analyses, bool WPA_MODE, bool Mem2Reg_MODE, bool PrintEdgeRecorder) :
										 		FinalResultsJson() {
		auto& lg = lg::get();
		BOOST_LOG_SEV(lg, INFO) << "Constructed the analysis controller.";
		BOOST_LOG_SEV(lg, INFO) << "Found the following IR files for this project: ";
    for (auto file : IRDB.source_files) {
      BOOST_LOG_SEV(lg, INFO) << "\t" << file;
		}
		// Check if the chosen entry points are valid
		BOOST_LOG_SEV(lg, INFO) << "Check for chosen entry points.";
		if (VariablesMap.count("entry_points")) {
			vector<string> invalidEntryPoints;
			for (auto &entryPoint : VariablesMap["entry_points"].as<vector<string>>()) {
				if (IRDB.getFunction(entryPoint) == nullptr) {
					invalidEntryPoints.push_back(entryPoint);
				}
			}
			if (invalidEntryPoints.size()) {
				for (auto &invalidEntryPoint : invalidEntryPoints) {
					BOOST_LOG_SEV(lg, ERROR) << "Entry point '" << invalidEntryPoint << "' is not valid.";
				}
				throw logic_error("invalid entry points");
			}
		}
    if (WPA_MODE) {
    	 // here we link every llvm module into a single module containing the entire IR
    	BOOST_LOG_SEV(lg, INFO) << "link all llvm modules into a single module for WPA ...\n";
    	IRDB.linkForWPA();
    }
    /*
     * Important
     * ---------
     * Note that if WPA_MODE was chosen by the user, the IRDB only contains one
     * single llvm::Module containing the whole program. For that reason all
     * subsequent loops are no real loops.
     */

    // here we perform a pre-analysis and run some very important passes over
    // all of the IR modules in order to perform various data flow analysis
    BOOST_LOG_SEV(lg, INFO) << "Start pre-analyzing modules.";
    for (auto& module_entry : IRDB.modules) {
      BOOST_LOG_SEV(lg, INFO) << "Pre-analyzing module: " << module_entry.first;
      llvm::Module& M = *(module_entry.second.get());
      llvm::LLVMContext& C = *(IRDB.contexts[module_entry.first].get());
      // TODO Have a look at this stuff from the future at some point in time
      /// PassManagerBuilder - This class is used to set up a standard optimization
      /// sequence for languages like C and C++, allowing some APIs to customize the
      /// pass sequence in various ways. A simple example of using it would be:
      ///
      ///  PassManagerBuilder Builder;
      ///  Builder.OptLevel = 2;
      ///  Builder.populateFunctionPassManager(FPM);
      ///  Builder.populateModulePassManager(MPM);
      ///
      /// In addition to setting up the basic passes, PassManagerBuilder allows
      /// frontends to vend a plugin API, where plugins are allowed to add extensions
      /// to the default pass manager.  They do this by specifying where in the pass
      /// pipeline they want to be added, along with a callback function that adds
      /// the pass(es).  For example, a plugin that wanted to add a loop optimization
      /// could do something like this:
      ///
      /// static void addMyLoopPass(const PMBuilder &Builder, PassManagerBase &PM) {
      ///   if (Builder.getOptLevel() > 2 && Builder.getOptSizeLevel() == 0)
      ///     PM.add(createMyAwesomePass());
      /// }
      ///   ...
      ///   Builder.addExtension(PassManagerBuilder::EP_LoopOptimizerEnd,
      ///                        addMyLoopPass);
      ///   ...
      // But for now, stick to what is well debugged
      llvm::legacy::PassManager PM;
      GeneralStatisticsPass* GSP = new GeneralStatisticsPass();
      ValueAnnotationPass* VAP = new ValueAnnotationPass(C);
      llvm::CFLSteensAAWrapperPass* SteensP = new llvm::CFLSteensAAWrapperPass();
      llvm::AAResultsWrapperPass* AARWP = new llvm::AAResultsWrapperPass();
      if (Mem2Reg_MODE) {
      	llvm::FunctionPass* Mem2Reg = llvm::createPromoteMemoryToRegisterPass();
      	PM.add(Mem2Reg);
      }
      PM.add(GSP);
      PM.add(VAP);
      PM.add(SteensP);
      PM.add(AARWP);
      PM.run(M);
      // just to be sure that none of the passes has messed up the module!
      bool broken_debug_info = false;
      if (llvm::verifyModule(M, &llvm::errs(), &broken_debug_info)) {
        BOOST_LOG_SEV(lg, CRITICAL) << "AnalysisController: module is broken!";
      }
      if (broken_debug_info) {
        BOOST_LOG_SEV(lg, WARNING) << "AnalysisController: debug info is broken.";
      }
      // Obtain the very important alias analysis results
      // and construct the intra-procedural points-to graphs.
      for (auto& function : M) {
				// When module-wise analysis is performed, declarations might occure
				// causing meaningless points-to graphs to be produced.
				if (!function.isDeclaration()) {
      		IRDB.ptgs.insert(make_pair(function.getName().str(), unique_ptr<PointsToGraph>(new PointsToGraph(AARWP->getAAResults(), &function))));
				}
			}
    }
    BOOST_LOG_SEV(lg, INFO) << "Pre-analysis completed.";
    IRDB.print();
    DBConn& db = DBConn::getInstance();
		db.synchronize(&IRDB);
    // Reconstruct the inter-modular class hierarchy and virtual function tables
    BOOST_LOG_SEV(lg, INFO) << "Reconstruct the class hierarchy.";
    LLVMStructTypeHierarchy CH(IRDB);
    BOOST_LOG_SEV(lg, INFO) << "Reconstruction of class hierarchy completed.";
		CH.printAsDot();
		// Set-up special summaries
		// IFDSSpecialSummaries<const llvm::Value*>& specialSummaries =
		//  		IFDSSpecialSummaries<const llvm::Value*>::getInstance();
		//   cout << specialSummaries << endl;

			// check and test the summary generation:
//      			cout << "GENERATE SUMMARY" << endl;
//      			LLVMIFDSSummaryGenerator<LLVMBasedICFG&, IFDSUnitializedVariables>
//      								Generator(M.getFunction("_Z6squarei"), icfg);
//      			auto summary = Generator.generateSummaryFlowFunction();

    /*
     * Perform whole program analysis (WPA) analysis
     * -----------
     */
    if (WPA_MODE) {
      LLVMBasedICFG ICFG(CH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF, [](){
						if (VariablesMap["entry_points"].as<vector<string>>().size())
							return VariablesMap["entry_points"].as<vector<string>>();
						else 
							return vector<string>({ "main" });
			}());
      ICFG.print();
      ICFG.printAsDot("interproc_cfg.dot");
	  	// CFG is only needed for intra-procedural monotone framework
      LLVMBasedCFG CFG;
      /*
       * Perform all the analysis that the user has chosen.
       */
      for (DataFlowAnalysisType analysis : Analyses) {
				BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis;
      	switch (analysis) {
      		case DataFlowAnalysisType::IFDS_TaintAnalysis:
       		{ // caution: observer '{' and '}' we work in another scope
       			IFDSTaintAnalysis taintanalysisproblem(ICFG);
       			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmtaintsolver(taintanalysisproblem, true);
       			llvmtaintsolver.solve();
       			break;
       		}
       		case DataFlowAnalysisType::IDE_TaintAnalysis:
       		{ // caution: observer '{' and '}' we work in another scope
       			IDETaintAnalysis taintanalysisproblem(ICFG);
       			LLVMIDESolver<const llvm::Value*, const llvm::Value*, LLVMBasedICFG&> llvmtaintsolver(taintanalysisproblem, true);
       			llvmtaintsolver.solve();
       			break;
       		}
       		case DataFlowAnalysisType::IFDS_TypeAnalysis:
       		{ // caution: observer '{' and '}' we work in another scope
       			IFDSTypeAnalysis typeanalysisproblem(ICFG);
       			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmtypesolver(typeanalysisproblem, true);
       			llvmtypesolver.solve();
       			break;
       		}
       		case DataFlowAnalysisType::IFDS_UninitializedVariables:
       		{ // caution: observer '{' and '}' we work in another scope
       			IFDSUnitializedVariables uninitializedvarproblem(ICFG);
       			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmunivsolver(uninitializedvarproblem, true);
						llvmunivsolver.solve();
						if(PrintEdgeRecorder)
							llvmunivsolver.exportJSONDataModel();
						// if (PrintEdgeRecorder) {
						// 	llvmunivsolver.dumpAllIntraPathEdges();
						// 	llvmunivsolver.dumpAllInterPathEdges();
						// }
       			break;
       		}
       		case DataFlowAnalysisType::IFDS_SolverTest:
       		{
       			IFDSSolverTest ifdstest(ICFG);
       			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmifdstestsolver(ifdstest, true);
       			llvmifdstestsolver.solve();
       			break;
       		}
       		case DataFlowAnalysisType::IDE_SolverTest:
       		{
       			IDESolverTest idetest(ICFG);
       			LLVMIDESolver<const llvm::Value*, const llvm::Value*, LLVMBasedICFG&> llvmidetestsolver(idetest, true);
       			llvmidetestsolver.solve();
       			break;
       		}
					case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation:
					{
						IntraMonoFullConstantPropagation intra(CFG, IRDB.getFunction("main"));
						LLVMIntraMonotoneSolver<pair<const llvm::Value*, unsigned>, LLVMBasedCFG&> solver(intra, true);
           	solver.solve();
						break;
					}
       		case DataFlowAnalysisType::MONO_Intra_SolverTest:
       		{
           	IntraMonotoneSolverTest intra(CFG, IRDB.getFunction("main"));
           	LLVMIntraMonotoneSolver<const llvm::Value*, LLVMBasedCFG&> solver(intra, true);
           	solver.solve();
       			break;
       		}
					case DataFlowAnalysisType::MONO_Inter_SolverTest:
					{
						InterMonotoneSolverTest inter(ICFG);
						LLVMInterMonotoneSolver<const llvm::Value*, LLVMBasedICFG&> solver(inter, true);
						solver.solve();
						break;
					}
					case DataFlowAnalysisType::Plugin:
					{
						break;
					}
       		case DataFlowAnalysisType::None:
					{
						break;
					}
       		default:
       			BOOST_LOG_SEV(lg, CRITICAL) << "The analysis it not valid";
       			break;
       		}
       }
    }
    /*
     * Perform module-wise (MW) analysis
     */
    else {
    	map<const llvm::Module*, LLVMBasedICFG> MWICFGs;
    	/*
       * We build all the call- and points-to graphs which can be used for
       * all of the analysis of course.
			 */
			BOOST_LOG_SEV(lg, INFO) << "Building module-wise icfgs";
			for (auto M : IRDB.getAllModules()) {
       	LLVMBasedICFG ICFG(CH, IRDB, *M, WalkerStrategy::Pointer, ResolveStrategy::OTF);
      	// store them away for later use
      	MWICFGs.insert(make_pair(M, ICFG));
      }
			BOOST_LOG_SEV(lg, INFO) << "Performing module-wise analysis";      

			// Some in-place testing
			// // start: module_wise_5 test
			// auto& Mod = *IRDB.getModuleDefiningFunction("main");
			// auto& Mod_2 = *IRDB.getModuleDefiningFunction("_Z3barPi");
			// LLVMBasedICFG ICFG(CH, IRDB, Mod, WalkerStrategy::Pointer, ResolveStrategy::OTF);
			// ICFG.printAsDot("icfg_main.dot");
			// ICFG.printInternalPTGAsDot("wptg_main.dot");
			// LLVMBasedICFG ICFG_2(CH, IRDB, Mod_2, WalkerStrategy::Pointer, ResolveStrategy::OTF);
			// ICFG_2.printAsDot("icfg_bar.dot");
			// ICFG_2.printInternalPTGAsDot("wptg_bar.dot");
			// cout << "@@@@@@@@@@@@@@@@@@@@@ call graph after merge @@@@@@@@@@@@@@@@@@@@@@\n";
			// ICFG.mergeWith(ICFG_2);
			// ICFG.printAsDot("icfg_after_merge.dot");
			// ICFG.printInternalPTGAsDot("wptg_after_merge.dot");
			// /// end: module_wise_5 test

			// // start: module_wise_6 test
			// auto& M_m = *IRDB.getModuleDefiningFunction("main");
			// auto& M_1 = *IRDB.getModuleDefiningFunction("_Z3fooRi");
			// auto& M_2 = *IRDB.getModuleDefiningFunction("_Z3barRi");
			// LLVMBasedICFG I(CH, IRDB, M_m, WalkerStrategy::Pointer, ResolveStrategy::OTF);
			// LLVMBasedICFG J(CH, IRDB, M_1, WalkerStrategy::Pointer, ResolveStrategy::OTF);
			// LLVMBasedICFG K(CH, IRDB, M_2, WalkerStrategy::Pointer, ResolveStrategy::OTF);
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
			// 				P.mergeWith(Q, llvm::ImmutableCallSite(Call), Call->getCalledFunction());
			// 			else if (i == 2)
			// 				P.mergeWith(R, llvm::ImmutableCallSite(Call), Call->getCalledFunction());
			// 		}
			// 	}
			// }
			// P.printAsDot("pure_final_ptg.dot");
			// auto& M_m = *IRDB.getModuleDefiningFunction("main");
			// auto& M_1 = *IRDB.getModuleDefiningFunction("_Z3fooRiS_S_");
			// LLVMBasedICFG I(CH, IRDB, M_m, WalkerStrategy::Pointer, ResolveStrategy::OTF);
			// LLVMBasedICFG J(CH, IRDB, M_1, WalkerStrategy::Pointer, ResolveStrategy::OTF);
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

       /*
        * Perform all the analysis that the user has chosen.
        */
      for (DataFlowAnalysisType analysis : Analyses) {
				for (auto &entry : MWICFGs) {
					const llvm::Module *M = entry.first;
					LLVMBasedICFG &I = entry.second;
					BOOST_LOG_SEV(lg, INFO) << "Performing analysis: " << analysis << " on module: " << M->getModuleIdentifier();
					switch (analysis) {
      			case DataFlowAnalysisType::IFDS_TaintAnalysis:
       			{
   	    			break;
   	    		}
   	    		case DataFlowAnalysisType::IDE_TaintAnalysis:
   	    		{ 
							 break;
   	    		}
   	    		case DataFlowAnalysisType::IFDS_TypeAnalysis:
   	    		{
   	    			break;
   	    		}
   	    		case DataFlowAnalysisType::IFDS_UninitializedVariables:
   	    		{ 
   	    			break;
   	    		}
   	    		case DataFlowAnalysisType::IFDS_SolverTest:
   	    		{
							IFDSSolverTest problem(I);
							LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG&> solver(problem, true);
							solver.solve();
   	    			break;
   	    		}
   	    		case DataFlowAnalysisType::IDE_SolverTest:
   	    		{
       			break;
       			}
						case DataFlowAnalysisType::MONO_Intra_FullConstantPropagation:
						{
						break;
						}
       			case DataFlowAnalysisType::MONO_Intra_SolverTest:
       			{
       				break;
					 	}
					 	case DataFlowAnalysisType::MONO_Inter_SolverTest:
						{
						break;
						}
       			case DataFlowAnalysisType::None:
						{
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
			BOOST_LOG_SEV(lg, INFO) << "Combining module-wise icfgs";
			auto M = IRDB.getModuleDefiningFunction("main");
			// LLVMBasedICFG I = MWICFGs[M];
			// for (auto &entry : MWICFGs) {
			// 	if (entry.first != M) {
			// 		I.mergeWith(entry.second);
			// 	}
			// }
			BOOST_LOG_SEV(lg, INFO) << "Combining module-wise results";
			// TODO Start at the main function and iterate over the entire program
			// combining all results!
      BOOST_LOG_SEV(lg, INFO) << "Combining module-wise results done, computation completed!";
    	BOOST_LOG_SEV(lg, INFO) << "Data-flow analyses completed.";
	}
}

void AnalysisController::writeResults(string filename) {
	ofstream ofs(filename, ofstream::app);
	ofs << FinalResultsJson.dump(1);
}

