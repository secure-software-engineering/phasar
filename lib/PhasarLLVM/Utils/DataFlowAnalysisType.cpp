/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>
#include <string>

#include "llvm/ADT/StringSwitch.h"

#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

using namespace psr;
using namespace std;

namespace psr {

std::string to_string(const DataFlowAnalysisType &D) {
  switch (D) {
  default:
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE)                          \
  case DataFlowAnalysisType::TYPE:                                             \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
  }
}

DataFlowAnalysisType to_DataFlowAnalysisType(const std::string &S) {
  DataFlowAnalysisType Type = llvm::StringSwitch<DataFlowAnalysisType>(S)
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE)                          \
  .Case(NAME, DataFlowAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
                                  .Default(DataFlowAnalysisType::None);
  if (Type == DataFlowAnalysisType::None) {
    Type = llvm::StringSwitch<DataFlowAnalysisType>(S)
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, TYPE)                          \
  .Case(CMDFLAG, DataFlowAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
               .Default(DataFlowAnalysisType::None);
  }
  return Type;
}

ostream &operator<<(ostream &os, const DataFlowAnalysisType &D) {
  return os << to_string(D);
}
} // namespace psr
