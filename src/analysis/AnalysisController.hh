#ifndef ANALYSISCONTROLLER_HH_
#define ANALYSISCONTROLLER_HH_

#include <llvm/Analysis/CFLAliasAnalysis.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#include <array>
#include <initializer_list>
#include <iostream>
#include <string>
#include "../db/DBConn.hh"
#include "../db/ProjectIRCompiledDB.hh"
#include "call-points-to_graph/PointsToGraph.hh"
#include "ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "ifds_ide/solver/LLVMIDESolver.hh"
#include "ifds_ide/solver/LLVMIFDSSolver.hh"
#include "passes/GeneralStatisticsPass.hh"
#include "passes/ValueAnnotationPass.hh"
// #include "ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.hh"
// #include "ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.hh"
// #include "ifds_ide_problems/ifds_type_analysis/IFDSTypeAnalysis.hh"
#include "ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.hh"
using namespace std;

enum class AnalysisKind {
  IFDS_TaintAnalysis = 0,
  IDE_TaintAnalysis,
  IFDS_TypeAnalysis,
  IFDS_UninitializedVariables
};

const array<string, 4> AnalysesNames = {
    {"IFDS_TaintAnalysis", "IDE_TaintAnalysis", "IFDS_TypeAnalysis",
     "IFDS_UninitializedVariables"}};

ostream& operator<<(ostream& os, const AnalysisKind& k) {
  int underlying_val = static_cast<std::underlying_type<AnalysisKind>::type>(k);
  return os << underlying_val << " - " << AnalysesNames.at(underlying_val);
}

class AnalysisController {
 public:
  AnalysisController(ProjectIRCompiledDB& IRDB,
                     initializer_list<AnalysisKind> Analyses) {
    cout << "constructed AnalysisController" << endl;
    cout << "found the following IR files for this project:" << endl;
    for (auto file : IRDB.source_files) {
      cout << "\t" << file << endl;
    }
    cout << "perform the following analyses on this project:" << endl;
    for (auto analysis : Analyses) {
      cout << "\t" << analysis << endl;
    }

    //	TODO lookup PassManagerBuilder Builder;
    //	TODO lookup Builder.populateModulePassManager();

    // just for testing
    map<string, unique_ptr<llvm::AAResults>> AAMap;

    // here we perform a pre-analysis and run some very important passes over
    // all of the IR modules in order to perform various data flow analysis
    for (auto& module_entry : IRDB.modules) {
      cout << "start analyzing module: " << module_entry.first << endl;
      llvm::Module& M = *(module_entry.second);
      llvm::LLVMContext& C = *(IRDB.contexts[module_entry.first]);
      llvm::legacy::PassManager PM;
      llvm::CFLAAWrapperPass* CFLAAPass = new llvm::CFLAAWrapperPass();
      llvm::AAResultsWrapperPass* AARWP = new llvm::AAResultsWrapperPass();
      PM.add(llvm::createPromoteMemoryToRegisterPass());
      PM.add(CFLAAPass);
      PM.add(AARWP);
      PM.add(new ValueAnnotationPass(C));
      PM.add(new GeneralStatisticsPass());
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
      // llvm::AAResults AARes(move(AARWP->getAAResults()));
      AAMap.insert(make_pair(M.getModuleIdentifier(),
                             unique_ptr<llvm::AAResults>(new llvm::AAResults(
                                 move(AARWP->getAAResults())))));
      M.dump();
    }

    cout << "AAMap size is: " << AAMap.size() << endl;
    llvm::AAResults* AAResult =
        AAMap["/home/pdschbrt/Schreibtisch/test/interproc_callsite.cpp"].get();

    // some very important pre-analyses are performed here, that have to store
    // the state for the whole project - that is for all IR modules making up
    // the entire project
    // create the function to module mapping to allow inter-modular analyses
    // (the function createFunctionModuleMappting() should be turned into a
    // module pass at some point)
    IRDB.createFunctionModuleMapping();
    IRDB.print();

    DBConn& db = DBConn::getInstance();
    // db << IRDB;

    // reconstruct the inter-modular class hierarchy and virtual function tables
    LLVMStructTypeHierarchy CH(IRDB);
    CH.print();

    db << CH;
    // db >> CH;

    // // prepare the ICFG the data-flow analyses are build on
    // cout << "starting data-flow analyses" << endl;
    // for (auto& module_entry : IRDB.modules) {
    //   llvm::Module& M = *(module_entry.second);
    //   LLVMBasedInterproceduralICFG icfg(M, *AAResult, CH, IRDB);
    //   llvm::Function* F = M.getFunction("main");
    //   cout << "PointsToGraph:" << endl;
    //   PointsToGraph ptg(*AAResult, F);
    //   ptg.print();
    //   //    cout << "CALLING WALKER!" << endl;
    //   //   icfg.resolveIndirectCallWalker(F);
    //   // create the analyses problems queried by the user and start analyzing

    //   // TODO: change the implementation of 'createZeroValue()'
    //   // The zeroValue can only be added one to a given context which means
    //   // a user can only create one analysis problem at a time, due to the
    //   // implementation of 'createZeroValue()'.

    //   for (auto analysis : Analyses) {
    //     switch (analysis) {
    //       case AnalysisKind::IFDS_TaintAnalysis:
    //         cout << "IFDS_TaintAnalysis\n";
    //         break;
    //       case AnalysisKind::IDE_TaintAnalysis:
    //         cout << "IDE_TaintAnalysis\n";
    //         break;
    //       case AnalysisKind::IFDS_TypeAnalysis:
    //         cout << "IFDS_TypeAnalysis\n";
    //         break;
    //       case AnalysisKind::IFDS_UninitializedVariables: {
    //         cout << "IFDS_UninitalizedVariables\n";
    //         // IFDSUnitializedVariables uninitializedvarproblem(
    //         //     icfg, *(IRDB.contexts[M.getModuleIdentifier()]));
    //         // LLVMIFDSSolver<const llvm::Value*, LLVMBasedInterproceduralICFG&>
    //         //     llvmunivsolver(uninitializedvarproblem, true);
    //         // llvmunivsolver.solve();
    //         break;
    //       }
    //       default:
    //         cout << "analysis not valid!" << endl;
    //         break;
    //     }
    //   }
    // }

    // // after every module has been analyzed the analyses results must be merged
    // // and the final results must be computed
    // cout << "combining module-wise results" << endl;

    // // here we go, now we are done
    // cout << "done analysis!" << endl;
  }
  ~AnalysisController() = default;
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
