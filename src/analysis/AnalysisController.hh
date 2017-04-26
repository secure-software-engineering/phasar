#ifndef ANALYSISCONTROLLER_HH_
#define ANALYSISCONTROLLER_HH_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#include <array>
#include <vector>
#include <initializer_list>
#include <iostream>
#include <string>
#include "../db/DBConn.hh"
#include "../db/ProjectIRCompiledDB.hh"
#include "call-points-to_graph/PointsToGraph.hh"
#include "ifds_ide/icfg/LLVMBasedICFG.hh"
#include "ifds_ide/solver/LLVMIDESolver.hh"
#include "ifds_ide/solver/LLVMIFDSSolver.hh"
#include "passes/GeneralStatisticsPass.hh"
#include "passes/ValueAnnotationPass.hh"
#include "ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.hh"
#include "ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.hh"
#include "ifds_ide_problems/ifds_type_analysis/IFDSTypeAnalysis.hh"
#include "ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.hh"
using namespace std;

enum class AnalysisType {
  IFDS_UninitializedVariables,
  IFDS_TaintAnalysis,
  IDE_TaintAnalysis,
  IFDS_TypeAnalysis,
};

extern const array<string, 4> AnalysesNames;

ostream& operator<<(ostream& os, const AnalysisType& k);

class AnalysisController {
 public:
  AnalysisController(ProjectIRCompiledDB& IRDB, vector<AnalysisType> Analyses, bool WPA_MODE=true);
  ~AnalysisController() = default;
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
