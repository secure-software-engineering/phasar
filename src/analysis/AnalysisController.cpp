#include "AnalysisController.hh"

// ostream& operator<<(ostream& os, const AnalysisKind& k) {
//   int underlying_val = static_cast<std::underlying_type<AnalysisKind>::type>(k);
//   return os << underlying_val << " - " << AnalysesNames.at(underlying_val);
// }

// AnalysisController::AnalysisController(
//     ProjectIRCompiledDB& IRDB, initializer_list<AnalysisKind> Analyses) {
  //   cout << "constructed controller" << endl;
  //   cout << "found the following IR files:" << endl;
  //   for (auto file : IRDB.source_files) {
  //     cout << "\t" << file << endl;
  //   }
  //   cout << "perform the following analyses" << endl;
  //   for (auto analysis : AnalysesNames) {
  //     cout << "\t" << analysis << endl;
  //   }
  //   //   //	PassManagerBuilder Builder;
  //   //   //	Builder.populateModulePassManager();
  //   //   llvm::legacy::PassManager PM;
  //   //   llvm::CFLAAWrapperPass* CFLAAPass = new llvm::CFLAAWrapperPass();
  //   //   llvm::AAResultsWrapperPass* AARWP = new
  //   llvm::AAResultsWrapperPass();
  //   //   PM.add(llvm::createPromoteMemoryToRegisterPass());
  //   //   PM.add(CFLAAPass);
  //   //   PM.add(AARWP);
  //   //   PM.add(new ValueAnnotationPass(C));
  //   //   PM.add(new GeneralStatisticsPass());
  //   //   PM.run(M);
  //   //   // just to be sure
  //   //   if (llvm::verifyModule(M)) {
  //   //     llvm::outs() << "ERROR: module is not valid: " <<
  //   M.getName().str()
  //   <<
  //   //     "\n";
  //   //     exit(-1);
  //   //   }
  //   //   llvm::outs() << M;
  //   //   llvm::AAResults& aaresults = AARWP->getAAResults();
  //   //   pti.analyzeModule(aaresults, M);
  //   //   cout << pti << endl;

  //   //   cout << "inter-procedural dependencies" << endl;
  //   //   LLVMBasedInterproceduralICFG icfg(M, aaresults, pti);
  //   //   const llvm::Function* F = M.getFunction("main");
  //   //   // cout << "CALLING WALKER" << endl;
  //   //   // icfg.resolveIndirectCallWalker(F);

  //   //   for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I
  //   !=
  //   //   E;
  //   //        ++I) {
  //   //     const llvm::Instruction& Inst = *I;
  //   //     if (llvm::isa<llvm::CallInst>(Inst) ||
  //   //     llvm::isa<llvm::InvokeInst>(Inst)) {
  //   //       auto possible_targets = icfg.getCalleesOfCallAt(&Inst);
  //   //       if (!possible_targets.empty()) {
  //   //         cout << "call to:" << endl;
  //   //         for (auto target : possible_targets) {
  //   //           cout << target->getName().str() << endl;
  //   //         }
  //   //       } else {
  //   //         cout << "EMPTY" << endl;
  //   //       }
  //   //     }
  //   //   }

  //   //	cout << "implementing the algorithm for circular dependencies" <<
  //   endl;
  //   //	LLVMBasedInterproceduralICFG icfg(M, pti);
  //   //	const llvm::Function* F = M.getFunction("main");
  //   //	for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I
  //   !=
  //   // E; ++I) {
  //   //		const llvm::Instruction& Inst = *I;
  //   //		if (llvm::isa<llvm::CallInst>(Inst) ||
  //   // llvm::isa<llvm::InvokeInst>(Inst)) {
  //   //			auto possible_targets =
  //   icfg.getCalleesOfCallAt(&Inst);
  //   //			if (!possible_targets.empty()) {
  //   //				cout << "call to:" << endl;
  //   //				for (auto target : possible_targets) {
  //   //					cout << target->getName().str() <<
  //   endl;
  //   //				}
  //   //				cout << "____" << endl;
  //   //			} else {
  //   //				cout << "EMPTY" << endl;
  //   //			}
  //   //		}
  //   //	}
// }