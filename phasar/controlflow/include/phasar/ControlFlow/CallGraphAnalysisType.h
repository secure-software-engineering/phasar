/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_CALLGRAPHANALYSISTYPE_H
#define PHASAR_CONTROLFLOW_CALLGRAPHANALYSISTYPE_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
enum class CallGraphAnalysisType {
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC) NAME,
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
  Invalid
};

std::string toString(CallGraphAnalysisType CGA);

CallGraphAnalysisType toCallGraphAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, CallGraphAnalysisType CGA);

} // namespace psr

#endif // PHASAR_CONTROLFLOW_CALLGRAPHANALYSISTYPE_H
