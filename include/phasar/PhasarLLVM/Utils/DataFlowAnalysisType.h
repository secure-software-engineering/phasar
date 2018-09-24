/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_DATAFLOWANALYSISTYPE_H_
#define PHASAR_PHASARLLVM_UTILS_DATAFLOWANALYSISTYPE_H_

#include <iosfwd>
#include <map>
#include <string>

namespace psr {

enum class DataFlowAnalysisType {
  IFDS_UninitializedVariables = 0,
  IFDS_ConstAnalysis,
  IFDS_TaintAnalysis,
  IDE_TaintAnalysis,
  IDE_TypeStateAnalysis,
  IFDS_TypeAnalysis,
  IFDS_SolverTest,
  IFDS_LinearConstantAnalysis,
  IDE_LinearConstantAnalysis,
  IDE_SolverTest,
  Intra_Mono_FullConstantPropagation,
  Intra_Mono_SolverTest,
  Inter_Mono_SolverTest,
  Inter_Mono_TaintAnalysis,
  Plugin,
  None
};

extern const std::map<std::string, DataFlowAnalysisType>
    StringToDataFlowAnalysisType;

extern const std::map<DataFlowAnalysisType, std::string>
    DataFlowAnalysisTypeToString;

std::ostream &operator<<(std::ostream &os, const DataFlowAnalysisType &k);

} // namespace psr

#endif
