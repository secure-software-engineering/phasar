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
#include <set>
#include <string>

namespace psr {

enum class DataFlowAnalysisType {
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
};

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;

std::string toString(const DataFlowAnalysisType &D);

DataFlowAnalysisType toDataFlowAnalysisType(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const DataFlowAnalysisType &D);

} // namespace psr

#endif
