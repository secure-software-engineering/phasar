#ifndef DATAFLOWANALYSISTYPE_H_
#define DATAFLOWANALYSISTYPE_H_

#include <map>
#include <string>
using namespace std;

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

extern const map<string, DataFlowAnalysisType> StringToDataFlowAnalysisType;

extern const map<DataFlowAnalysisType, string> DataFlowAnalysisTypeToString;

ostream &operator<<(ostream &os, const DataFlowAnalysisType &k);

#endif
