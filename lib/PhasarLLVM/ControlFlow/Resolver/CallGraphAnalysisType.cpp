/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

std::string psr::toString(CallGraphAnalysisType CGA) {
  switch (CGA) {
  default:
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE)                     \
  case CallGraphAnalysisType::TYPE:                                            \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  }
}

psr::CallGraphAnalysisType psr::toCallGraphAnalysisType(llvm::StringRef S) {
  CallGraphAnalysisType Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE)                     \
  .Case(NAME, CallGraphAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
                                   .Default(CallGraphAnalysisType::Invalid);
  if (Type == CallGraphAnalysisType::Invalid) {
    Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE)                     \
  .Case(CMDFLAG, CallGraphAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
               .Default(CallGraphAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   CallGraphAnalysisType CGA) {
  return OS << toString(CGA);
}
