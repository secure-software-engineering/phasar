#pragma once

#include <vector>

#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>

namespace psr {

class ProjectIRDB;

void analyzeCoreUtilsUsingPreAnalysis(ProjectIRDB &&IRDB,
                                      std::vector<DataFlowAnalysisType> Analyses);

void analyzeCoreUtilsWithoutUsingPreAnalysis(
    ProjectIRDB &&IRDB, std::vector<DataFlowAnalysisType> Analyses);

} // namespace psr
