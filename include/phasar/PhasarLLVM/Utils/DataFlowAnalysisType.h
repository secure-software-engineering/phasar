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

#include <set>
#include <string>
#include <variant>

#include "phasar/PhasarLLVM/Plugins/PluginCtors.h"

namespace llvm {
class raw_ostream;
}

namespace psr {

enum class DataFlowAnalysisType {
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
};

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;
using DataFlowAnalysisKind =
    std::variant<DataFlowAnalysisType, IDEPluginConstructor,
                 IFDSPluginConstructor, IntraMonoPluginConstructor,
                 InterMonoPluginConstructor>;

std::string toString(const DataFlowAnalysisType &D);

DataFlowAnalysisType toDataFlowAnalysisType(const std::string &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const DataFlowAnalysisType &D);

} // namespace psr

#endif
