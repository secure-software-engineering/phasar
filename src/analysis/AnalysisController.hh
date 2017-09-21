#ifndef ANALYSISCONTROLLER_HH_
#define ANALYSISCONTROLLER_HH_

#include "../db/DBConn.hh"
#include "../db/PHSStringConverter.hh"
#include "../db/ProjectIRCompiledDB.hh"
#include "../utils/Logger.hh"
#include "../utils/SOL.hh"
#include "call-points-to_graph/PointsToGraph.hh"
#include "icfg/LLVMBasedCFG.hh"
#include "icfg/LLVMBasedICFG.hh"
#include "ifds_ide/LLVMIFDSSummaryGenerator.hh"
#include "ifds_ide/solver/LLVMIDESolver.hh"
#include "ifds_ide/solver/LLVMIFDSSolver.hh"
#include "ifds_ide_problems/ide_solver_test/IDESolverTest.hh"
#include "ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.hh"
#include "ifds_ide_problems/ifds_solver_test/IFDSSolverTest.hh"
#include "ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.hh"
#include "ifds_ide_problems/ifds_type_analysis/IFDSTypeAnalysis.hh"
#include "ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.hh"
#include "plugins/plugin_ifaces/IFDSTabulationProblemPlugin.hh"
#include "plugins/plugin_ifaces/IDETabulationProblemPlugin.hh"
#include "plugins/plugin_ifaces/InterMonotoneProblemPlugin.hh"
#include "plugins/plugin_ifaces/IntraMonotoneProblemPlugin.hh"
#include "plugins/AnalysisPlugin.hh"
#include "json.hpp"
#include "monotone/solver/LLVMInterMonotoneSolver.hh"
#include "monotone/solver/LLVMIntraMonotoneSolver.hh"
#include "monotone_problems/inter_monotone_solver_test/InterMonotoneSolverTest.hh"
#include "monotone_problems/intra_full_constant_propagation/IntraMonoFullConstantPropagation.hh"
#include "monotone_problems/intra_monotone_solver_test/IntraMonotoneSolverTest.hh"
#include "passes/GeneralStatisticsPass.hh"
#include "passes/ValueAnnotationPass.hh"
#include <array>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <string>
#include <vector>

using namespace std;
using json = nlohmann::json;

enum class DataFlowAnalysisType {
  IFDS_UninitializedVariables = 0,
  IFDS_TaintAnalysis,
  IDE_TaintAnalysis,
  IFDS_TypeAnalysis,
  IFDS_SolverTest,
  IDE_SolverTest,
  MONO_Intra_FullConstantPropagation,
  MONO_Intra_SolverTest,
  MONO_Inter_SolverTest,
  Plugin,
  None
};

extern const map<string, DataFlowAnalysisType> DataFlowAnalysisTypeMap;

ostream &operator<<(ostream &os, const DataFlowAnalysisType &k);

enum class ExportType { JSON = 0 };

extern const map<string, ExportType> ExportTypeMap;

ostream &operator<<(ostream &os, const ExportType &e);

class AnalysisController {
private:
  json FinalResultsJson;

public:
  AnalysisController(ProjectIRCompiledDB &&IRDB,
                     vector<DataFlowAnalysisType> Analyses,
                     bool WPA_MODE = true, bool Mem2Reg_MODE = true,
                     bool PrintEdgeRecorder = true);
  ~AnalysisController() = default;
  void writeResults(string filename);
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
