/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"

std::string psr::toString(CallGraphAnalysisType CGA) {
  switch (CGA) {
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  case CallGraphAnalysisType::NAME:                                            \
    return #NAME;
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
  case CallGraphAnalysisType::Invalid:
    return "Invalid";
  }
}

psr::CallGraphAnalysisType psr::toCallGraphAnalysisType(llvm::StringRef S) {
  CallGraphAnalysisType Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  .Case(#NAME, CallGraphAnalysisType::NAME)
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
                                   .Default(CallGraphAnalysisType::Invalid);
  if (Type == CallGraphAnalysisType::Invalid) {
    Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  .Case(CMDFLAG, CallGraphAnalysisType::NAME)
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
               .Default(CallGraphAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   CallGraphAnalysisType CGA) {
  return OS << toString(CGA);
}
