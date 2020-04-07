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
#include <string>

namespace psr {

enum class DataFlowAnalysisType {
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
};

std::string to_string(const DataFlowAnalysisType &D);

DataFlowAnalysisType to_DataFlowAnalysisType(const std::string &S);

std::ostream &operator<<(std::ostream &os, const DataFlowAnalysisType &D);

} // namespace psr

#endif
