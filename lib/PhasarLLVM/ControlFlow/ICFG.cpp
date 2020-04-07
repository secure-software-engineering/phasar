/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ICFG.cpp
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#include <ostream>
#include <string>

#include "llvm/ADT/StringSwitch.h"

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"

using namespace psr;
using namespace std;

namespace psr {

std::string toString(const CallGraphAnalysisType &CGA) {
  switch (CGA) {
  default:
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE)                     \
  case CallGraphAnalysisType::TYPE:                                            \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  }
}

CallGraphAnalysisType toCallGraphAnalysisType(const std::string &S) {
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

ostream &operator<<(ostream &OS, const CallGraphAnalysisType &CGA) {
  return OS << toString(CGA);
}

} // namespace psr
