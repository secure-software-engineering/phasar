/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ANALYSISCONTROLLER_HH_
#define ANALYSISCONTROLLER_HH_

#include "../db/DBConn.h"
#include "../db/ProjectIRDB.h"
#include "../utils/Logger.h"
#include "../utils/SOL.h"
#include "control_flow/LLVMBasedCFG.h"
#include "control_flow/LLVMBasedICFG.h"
#include "ifds_ide/IDESummaries.h"
#include "ifds_ide/LLVMIFDSSummaryGenerator.h"
#include "ifds_ide/solver/IDESummaryGenerator.h"
#include "ifds_ide/solver/LLVMIDESolver.h"
#include "ifds_ide/solver/LLVMIFDSSolver.h"
#include "ifds_ide/solver/LLVMMWAIDESolver.h"
#include "ifds_ide/solver/LLVMMWAIFDSSolver.h"
#include "ifds_ide_problems/ide_solver_test/IDESolverTest.h"
#include "ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.h"
#include "ifds_ide_problems/ifds_const_analysis/IFDSConstAnalysis.h"
#include "ifds_ide_problems/ifds_solver_test/IFDSSolverTest.h"
#include "ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.h"
#include "ifds_ide_problems/ifds_type_analysis/IFDSTypeAnalysis.h"
#include "ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.h"
#include "json.hpp"
#include "misc/DataFlowAnalysisType.h"
#include "misc/SummaryStrategy.h"
#include "monotone/solver/LLVMInterMonotoneSolver.h"
#include "monotone/solver/LLVMIntraMonotoneSolver.h"
#include "monotone_problems/inter_monotone_solver_test/InterMonotoneSolverTest.h"
#include "monotone_problems/intra_full_constant_propagation/IntraMonoFullConstantPropagation.h"
#include "monotone_problems/intra_monotone_solver_test/IntraMonotoneSolverTest.h"
#include "plugins/AnalysisPluginController.h"
#include "plugins/plugin_ifaces/control_flow/ICFGPlugin.h"
#include "points-to/PointsToGraph.h"
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

enum class ExportType { JSON = 0 };

extern const map<string, ExportType> StringToExportType;

extern const map<ExportType, string> ExportTypeToString;

ostream &operator<<(ostream &os, const ExportType &e);

class AnalysisController {
private:
  json FinalResultsJson;

public:
  AnalysisController(ProjectIRDB &&IRDB, vector<DataFlowAnalysisType> Analyses,
                     bool WPA_MODE = true, bool PrintEdgeRecorder = true,
                     string graph_id = "");
  ~AnalysisController() = default;
  void writeResults(string filename);
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
