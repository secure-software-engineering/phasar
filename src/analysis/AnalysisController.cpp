#include "AnalysisController.hh"

const array<string, 4> AnalysesNames = {
  {"IFDS_UninitializedVariables",
   "IFDS_TaintAnalysis",
   "IDE_TaintAnalysis",
   "IFDS_TypeAnalysis",
}};

ostream& operator<<(ostream& os, const AnalysisType& k) {
  int underlying_val = static_cast<std::underlying_type<AnalysisType>::type>(k);
  return os << underlying_val << " - " << AnalysesNames.at(underlying_val);
}

  AnalysisController::AnalysisController(ProjectIRCompiledDB& IRDB,
                     vector<AnalysisType> Analyses, bool WPA_MODE) {
    cout << "constructed AnalysisController ...\n";
    cout << "found the following IR files for this project:" << endl;
    for (auto file : IRDB.source_files) {
      cout << "\t" << file << endl;
    }
    cout << "WPA_MODE: " << WPA_MODE << "\n";
    if (WPA_MODE) {
    	 // here we link every llvm module into a single module containing the entire IR
    	cout << "link all llvm modules into a single module for WPA ...\n";
    	IRDB.linkForWPA();
    }
    // here we perform a pre-analysis and run some very important passes over
    // all of the IR modules in order to perform various data flow analysis
    cout << "start pre-analyzing modules ...\n";
    for (auto& module_entry : IRDB.modules) {
      cout << "pre-analyzing module: " << module_entry.first << "\n";
      llvm::Module& M = *(module_entry.second.get());
      llvm::LLVMContext& C = *(IRDB.contexts[module_entry.first].get());
      // TODO: Have a look at this stuff from the future at some point in time
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
      llvm::FunctionPass* Mem2Reg = llvm::createPromoteMemoryToRegisterPass();
      GeneralStatisticsPass* GSP = new GeneralStatisticsPass();
      ValueAnnotationPass* VAP = new ValueAnnotationPass(C);
      llvm::CFLSteensAAWrapperPass* SteensP = new llvm::CFLSteensAAWrapperPass();
      llvm::AAResultsWrapperPass* AARWP = new llvm::AAResultsWrapperPass();
      PM.add(Mem2Reg);
      PM.add(GSP);
      PM.add(VAP);
      PM.add(SteensP);
      PM.add(AARWP);
      PM.run(M);
      // just to be sure that none of the passes has messed up the module!
      bool broken_debug_info = false;
      if (llvm::verifyModule(M, &llvm::errs(), &broken_debug_info)) {
        cout << "AnalysisController: module is broken!" << endl;
      }
      if (broken_debug_info) {
        cout << "AnalysisController: debug info is broken" << endl;
      }
      // obtain the very important alias analysis results
      // and construct the intra-procedural points-to graphs
      for (auto& function : M) {
      	IRDB.ptgs.insert(make_pair(function.getName().str(), unique_ptr<PointsToGraph>(new PointsToGraph(AARWP->getAAResults(), &function))));
      }
    }
    cout << "pre-analysis completed ...\n";
    IRDB.print();

    DBConn& db = DBConn::getInstance();
    // db << IRDB;

    // reconstruct the inter-modular class hierarchy and virtual function tables
    cout << "reconstruction the class hierarchy ...\n";
    LLVMStructTypeHierarchy CH(IRDB);
    cout << "reconstruction completed ...\n";
    CH.print();
    CH.printAsDot();

    // db << CH;
    // db >> CH;

    SpecialSummaries<const llvm::Value*>& specialSummaries =
    		SpecialSummaries<const llvm::Value*>::getInstance();
    cout << specialSummaries << endl;

   	// prepare the ICFG the data-flow analyses are build on
    cout << "starting the chosen data-flow analyses ...\n";
    for (auto& module_entry : IRDB.modules) {
    	// create the analyses problems queried by the user and start analyzing
    	llvm::Module& M = *(module_entry.second);
    	llvm::LLVMContext& C = *IRDB.getLLVMContext(M.getModuleIdentifier());
    	LLVMBasedICFG icfg(M, CH, IRDB);
    	icfg.print();
      // TODO: change the implementation of 'createZeroValue()'
      // The zeroValue can only be added one to a given context which means
      // a user can only create one analysis problem at a time, due to the
      // implementation of 'createZeroValue()'.
      // so it would be nice to check if the zerovalue already exists and if so
      // just return it!
      for (auto analysis : Analyses) {
      	switch (analysis) {
      		case AnalysisType::IFDS_TaintAnalysis:
      		{ // caution: observer '{' and '}' we work in another scope
      			cout << "IFDS_TaintAnalysis\n";
      			IFDSTaintAnalysis taintanalysisproblem(icfg, M.getContext());
      			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmtaintsolver(taintanalysisproblem, true);
      			llvmtaintsolver.solve();
      			break;
      		}
      		case AnalysisType::IDE_TaintAnalysis:
      		{ // caution: observer '{' and '}' we work in another scope
      			cout << "IDE_TaintAnalysis\n";
      			//  IDETaintAnalysis taintanalysisproblem(icfg, *(IRDB.contexts[M.getModuleIdentifier()]));
      			//  LLVMIDESolver<const llvm::Value*, LLVMBasedInterproceduralICFG&> llvmtaintsolver(taintanalysisproblem, true);
      			//  llvmtaintsolver.solve();
      			break;
      		}
      		case AnalysisType::IFDS_TypeAnalysis:
      		{ // caution: observer '{' and '}' we work in another scope
      			cout << "IFDS_TypeAnalysis\n";
      			IFDSTypeAnalysis typeanalysisproblem(icfg, M.getContext());
      			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmtypesolver(typeanalysisproblem, true);
      			llvmtypesolver.solve();
      			break;
      		}
      		case AnalysisType::IFDS_UninitializedVariables:
      		{ // caution: observer '{' and '}' we work in another scope
      			cout << "IFDS_UninitalizedVariables\n";
      			IFDSUnitializedVariables uninitializedvarproblem(icfg, M.getContext());
      			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> llvmunivsolver(uninitializedvarproblem, true);
      			llvmunivsolver.solve();

      			// check and test the summary generation:
//      			cout << "GENERATE SUMMARY" << endl;
//      			IFDSSummaryGenerator<const llvm::Value*, LLVMBasedICFG&, IFDSUnitializedVariables>
//      								Generator(M.getFunction("_Z6squarei"), icfg, C);
//      			auto summary = Generator.generateSummaryFlowFunction();
      			break;
      		}
      		default:
      			cout << "analysis not valid!" << endl;
      			break;
      	}
      }
      if (!WPA_MODE) {
      	// after every module has been analyzed the analyses results must be
        // merged and the final results must be computed
        cout << "combining module-wise results ...\n";

        // here we go, now we are done
        cout << "combining module-wise results done ...\n"
        				"computation completed!\n";
      }
    }
    cout << "data-flow analyses completed ...\n";
}

