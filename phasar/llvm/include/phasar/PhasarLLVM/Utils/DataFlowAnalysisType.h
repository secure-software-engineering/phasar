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

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {

enum class DataFlowAnalysisType {
  None,
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, DESC) NAME,
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
};

std::string toString(DataFlowAnalysisType D);

DataFlowAnalysisType toDataFlowAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, DataFlowAnalysisType D);

} // namespace psr

#endif
