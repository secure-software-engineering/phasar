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
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>

using json = nlohmann::json;

enum class ExportType { JSON = 0 };

extern const std::map<std::string, ExportType> StringToExportType;

extern const std::map<ExportType, std::string> ExportTypeToString;

ostream &operator<<(std::ostream &os, const ExportType &e);

class AnalysisController {
private:
  json FinalResultsJson;

public:
  AnalysisController(ProjectIRDB &&IRDB, vector<DataFlowAnalysisType> Analyses,
                     bool WPA_MODE = true, bool PrintEdgeRecorder = true,
                     std::string graph_id = "");
  ~AnalysisController() = default;
  void writeResults(std::string filename);
};

#endif /* ANALYSIS_ANALYSISCONTROLLER_HH_ */
