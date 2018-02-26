#ifndef WWOCOREUTILSPREANALYSIS_H_
#define WWOCOREUTILSPREANALYSIS_H_

#include <phasar/Config/Configuration.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/Mono/Problems/IntraMonotoneSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonotoneSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonotoneSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMIntraMonotoneSolver.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
#include <phasar/PhasarLLVM/Plugins/AnalysisPluginController.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/SOL.h>
#include <chrono>
#include <iostream>

void analyzeCoreUtilsUsingPreAnalysis(ProjectIRDB &&IRDB,
                                      vector<DataFlowAnalysisType> Analyses);

void analyzeCoreUtilsWithoutUsingPreAnalysis(
    ProjectIRDB &&IRDB, vector<DataFlowAnalysisType> Analyses);

#endif
