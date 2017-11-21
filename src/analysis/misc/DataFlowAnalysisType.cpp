#include "DataFlowAnalysisType.h"

const map<string, DataFlowAnalysisType> StringToDataFlowAnalysisType = {
    {"ifds_uninit", DataFlowAnalysisType::IFDS_UninitializedVariables},
    {"ifds_taint", DataFlowAnalysisType::IFDS_TaintAnalysis},
    {"ifds_type", DataFlowAnalysisType::IFDS_TypeAnalysis},
    {"ide_taint", DataFlowAnalysisType::IDE_TaintAnalysis},
    {"ifds_solvertest", DataFlowAnalysisType::IFDS_SolverTest},
    {"ide_solvertest", DataFlowAnalysisType::IDE_SolverTest},
    {"mono_intra_fullconstpropagation",
     DataFlowAnalysisType::MONO_Intra_FullConstantPropagation},
    {"mono_intra_solvertest", DataFlowAnalysisType::MONO_Intra_SolverTest},
    {"mono_inter_solvertest", DataFlowAnalysisType::MONO_Inter_SolverTest},
    {"plugin", DataFlowAnalysisType::Plugin},
    {"none", DataFlowAnalysisType::None}};

const map<DataFlowAnalysisType, string> DataFlowAnalysisTypeToString = {
    {DataFlowAnalysisType::IFDS_UninitializedVariables, "ifds_uninit"},
    {DataFlowAnalysisType::IFDS_TaintAnalysis, "ifds_taint"},
    {DataFlowAnalysisType::IFDS_TypeAnalysis, "ifds_type"},
    {DataFlowAnalysisType::IDE_TaintAnalysis, "ide_taint"},
    {DataFlowAnalysisType::IFDS_SolverTest, "ifds_solvertest"},
    {DataFlowAnalysisType::IDE_SolverTest, "ide_solvertest"},
    {DataFlowAnalysisType::MONO_Intra_FullConstantPropagation,
     "mono_intra_fullconstpropagation"},
    {DataFlowAnalysisType::MONO_Intra_SolverTest, "mono_intra_solvertest"},
    {DataFlowAnalysisType::MONO_Inter_SolverTest, "mono_inter_solvertest"},
    {DataFlowAnalysisType::Plugin, "plugin"},
    {DataFlowAnalysisType::None, "none"}};

ostream &operator<<(ostream &os, const DataFlowAnalysisType &D) {
  return os << DataFlowAnalysisTypeToString.at(D);
}
