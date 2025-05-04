/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

std::string psr::toString(DataFlowAnalysisType D) {
  switch (D) {
  default:
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, DESC)                          \
  case DataFlowAnalysisType::NAME:                                             \
    return #NAME;                                                              \
    break;
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
  case DataFlowAnalysisType::None:
    return "None";
  }
}

psr::DataFlowAnalysisType psr::toDataFlowAnalysisType(llvm::StringRef S) {
  DataFlowAnalysisType Type = llvm::StringSwitch<DataFlowAnalysisType>(S)
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, DESC)                          \
  .Case(#NAME, DataFlowAnalysisType::NAME)
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
                                  .Default(DataFlowAnalysisType::None);
  if (Type == DataFlowAnalysisType::None) {
    Type = llvm::StringSwitch<DataFlowAnalysisType>(S)
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, DESC)                          \
  .Case(CMDFLAG, DataFlowAnalysisType::NAME)
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
               .Default(DataFlowAnalysisType::None);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   DataFlowAnalysisType D) {
  return OS << toString(D);
}
