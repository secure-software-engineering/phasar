/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CALLGRAPHANALYSISTYPE_H
#define PHASAR_PHASARLLVM_CALLGRAPHANALYSISTYPE_H

#include "llvm/ADT/StringRef.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {
enum class CallGraphAnalysisType {
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  Invalid
};

std::string toString(CallGraphAnalysisType CGA);

CallGraphAnalysisType toCallGraphAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, CallGraphAnalysisType CGA);

} // namespace psr

#endif // PHASAR_PHASARLLVM_CALLGRAPHANALYSISTYPE_H
