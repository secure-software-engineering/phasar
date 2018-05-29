/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef DATAFLOWANALYSISTYPE_H_
#define DATAFLOWANALYSISTYPE_H_

#include <iostream>
#include <map>
#include <string>
using namespace std;

namespace psr{

enum class DataFlowAnalysisType {
  IFDS_UninitializedVariables = 0,
  IFDS_ConstAnalysis,
  IFDS_TaintAnalysis,
  IDE_TaintAnalysis,
  IDE_TypeStateAnalysis,
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

}//namespace psr

#endif
