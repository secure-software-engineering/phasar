#ifndef WWOCOREUTILSPREANALYSIS_H_
#define WWOCOREUTILSPREANALYSIS_H_

#include "../analysis/control_flow/LLVMBasedICFG.h"
#include "../analysis/ifds_ide/solver/LLVMIDESolver.h"
#include "../analysis/ifds_ide/solver/LLVMIFDSSolver.h"
#include "../analysis/ifds_ide_problems/ide_solver_test/IDESolverTest.h"
#include "../analysis/ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.h"
#include "../analysis/ifds_ide_problems/ifds_solver_test/IFDSSolverTest.h"
#include "../analysis/ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.h"
#include "../analysis/ifds_ide_problems/ifds_type_analysis/IFDSTypeAnalysis.h"
#include "../analysis/ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.h"
#include "../analysis/misc/DataFlowAnalysisType.h"
#include "../analysis/monotone/solver/LLVMInterMonotoneSolver.h"
#include "../analysis/monotone/solver/LLVMIntraMonotoneSolver.h"
#include "../analysis/monotone_problems/inter_monotone_solver_test/InterMonotoneSolverTest.h"
#include "../analysis/monotone_problems/intra_full_constant_propagation/IntraMonoFullConstantPropagation.h"
#include "../analysis/monotone_problems/intra_monotone_solver_test/IntraMonotoneSolverTest.h"
#include "../analysis/plugins/AnalysisPluginController.h"
#include "../config/Configuration.h"
#include "../db/ProjectIRDB.h"
#include "../utils/Logger.h"
#include "../utils/SOL.h"
#include <chrono>
#include <iostream>

void analyzeCoreUtilsUsingPreAnalysis(ProjectIRDB &&IRDB,
                                      vector<DataFlowAnalysisType> Analyses);

void analyzeCoreUtilsWithoutUsingPreAnalysis(
    ProjectIRDB &&IRDB, vector<DataFlowAnalysisType> Analyses);

#endif
