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
#include "call-points-to_graph/PointsToInformation.hh"
#include "ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "passes/GeneralStatisticsPass.hh"
#include "passes/ValueAnnotationPass.hh"
#include "ifds_ide/solver/LLVMIFDSSolver.hh"
#include "ifds_ide/solver/LLVMIDESolver.hh"
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
 private:
  PointsToInformation pti;

 public:
  AnalysisController(ProjectIRCompiledDB& IRDB,
                     initializer_list<AnalysisKind> Analyses) {
    cout << "constructed controller" << endl;
    cout << "found the following IR files:" << endl;
    for (auto file : IRDB.source_files) {
      cout << "\t" << file << endl;
    }
    cout << "perform the following analyses" << endl;
    for (auto analysis : Analyses) {
      cout << "\t" << analysis << endl;
    }
    //	TODO lookup PassManagerBuilder Builder;
    //	TODO lookup Builder.populateModulePassManager();
    // here every module undergoes the static analysis
    cout << "analyzing modules" << endl;
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
      M.dump();
      // obtain the alias analysis results
      llvm::AAResults& AARes = AARWP->getAAResults();
      pti.analyzeModule(AARes, M);
      cout << pti << endl;

      // cout << "inter-procedural dependencies" << endl;
      // LLVMBasedInterproceduralICFG icfg(M, AARes, pti);
      // const llvm::Function* F = M.getFunction("main");
      // cout << "CALLING WALKER" << endl;
      // //   //   // icfg.resolveIndirectCallWalker(F);

      // for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I !=
      // E;
      //      ++I) {
      //   const llvm::Instruction& Inst = *I;
      // if (llvm::isa<llvm::CallInst>(Inst) ||
      //     llvm::isa<llvm::InvokeInst>(Inst)) {
      //   auto possible_targets = icfg.getCalleesOfCallAt(&Inst);
      //   if (!possible_targets.empty()) {
      //     cout << "call to:" << endl;
      //     for (auto target : possible_targets) {
      //       cout << target->getName().str() << endl;
      //     }
      //   } else {
      //     cout << "EMPTY" << endl;
      //   }
      // }

      // TODO: change the implementation of 'createZeroValue()'
      // The zeroValue can only be added one to a given context which means
      // a user can only create one analysis problem at a time, due to the
      // implementation of 'createZeroValue()'.

      LLVMBasedInterproceduralICFG icfg(M, AARes, pti);
      IFDSUnitializedVariables uninitializedvarproblem(icfg, C);
      LLVMIFDSSolver<const llvm::Value*, LLVMBasedInterproceduralICFG&>
          llvmunivsolver(uninitializedvarproblem, true);
      llvmunivsolver.solve();
      cout << "finished analyzing module: " << module_entry.first << endl;
    }
    // after every module has been analyzed the analyses results must be merged
    // and the final results must be computed
    cout << "combining module-wise results" << endl;
  }
  ~AnalysisController() = default;
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
