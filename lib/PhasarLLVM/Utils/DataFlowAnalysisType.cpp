/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
using namespace psr;
using namespace std;

namespace psr {

const map<string, DataFlowAnalysisType> StringToDataFlowAnalysisType = {
    {"ifds-const", DataFlowAnalysisType::IFDS_ConstAnalysis},
    {"ifds-lca", DataFlowAnalysisType::IFDS_LinearConstantAnalysis},
    {"ifds-solvertest", DataFlowAnalysisType::IFDS_SolverTest},
    {"ifds-taint", DataFlowAnalysisType::IFDS_TaintAnalysis},
    {"ifds-type", DataFlowAnalysisType::IFDS_TypeAnalysis},
    {"ifds-uninit", DataFlowAnalysisType::IFDS_UninitializedVariables},
    {"ide-lca", DataFlowAnalysisType::IDE_LinearConstantAnalysis},
    {"ide-solvertest", DataFlowAnalysisType::IDE_SolverTest},
    {"ide-taint", DataFlowAnalysisType::IDE_TaintAnalysis},
    {"ide-typestate", DataFlowAnalysisType::IDE_TypeStateAnalysis},
    {"intra-mono-fullconstpropagation",
     DataFlowAnalysisType::Intra_Mono_FullConstantPropagation},
    {"intra-mono-solvertest", DataFlowAnalysisType::Intra_Mono_SolverTest},
    {"inter-mono-solvertest", DataFlowAnalysisType::Inter_Mono_SolverTest},
    {"inter-mono-taint", DataFlowAnalysisType::Inter_Mono_TaintAnalysis},
    {"plugin", DataFlowAnalysisType::Plugin},
    {"none", DataFlowAnalysisType::None}};

const map<DataFlowAnalysisType, string> DataFlowAnalysisTypeToString = {
    {DataFlowAnalysisType::IFDS_ConstAnalysis, "ifds-const"},
    {DataFlowAnalysisType::IFDS_LinearConstantAnalysis, "ifds-lca"},
    {DataFlowAnalysisType::IFDS_SolverTest, "ifds-solvertest"},
    {DataFlowAnalysisType::IFDS_TaintAnalysis, "ifds-taint"},
    {DataFlowAnalysisType::IFDS_TypeAnalysis, "ifds-type"},
    {DataFlowAnalysisType::IFDS_UninitializedVariables, "ifds-uninit"},
    {DataFlowAnalysisType::IDE_LinearConstantAnalysis, "ide-lca"},
    {DataFlowAnalysisType::IDE_SolverTest, "ide-solvertest"},
    {DataFlowAnalysisType::IDE_TaintAnalysis, "ide-taint"},
    {DataFlowAnalysisType::IDE_TypeStateAnalysis, "ide-typestate"},
    {DataFlowAnalysisType::Intra_Mono_FullConstantPropagation,
     "intra-mono-fullconstpropagation"},
    {DataFlowAnalysisType::Intra_Mono_SolverTest, "intra-mono-solvertest"},
    {DataFlowAnalysisType::Inter_Mono_SolverTest, "inter-mono-solvertest"},
    {DataFlowAnalysisType::Inter_Mono_TaintAnalysis, "inter-mono-taint"},
    {DataFlowAnalysisType::Plugin, "plugin"},
    {DataFlowAnalysisType::None, "none"}};

ostream &operator<<(ostream &os, const DataFlowAnalysisType &D) {
  return os << DataFlowAnalysisTypeToString.at(D);
}
} // namespace psr
