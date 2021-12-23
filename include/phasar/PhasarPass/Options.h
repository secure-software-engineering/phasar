/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARPASS_OPTIONS_H_
#define PHASAR_PHASARPASS_OPTIONS_H_

#include <string>
#include <vector>

#include "llvm/Support/CommandLine.h"

extern llvm::cl::OptionCategory PhASARCategory; // NOLINT

namespace psr {

extern std::string DataFlowAnalysis; // NOLINT

extern std::string PointerAnalysis; // NOLINT

extern std::string CallGraphAnalysis; // NOLINT

extern std::vector<std::string> EntryPoints; // NOLINT

extern std::string PammOutputFile; // NOLINT

extern bool PrintEdgeRecorder; // NOLINT

extern bool InitLogger; // NOLINT

extern bool DumpResults; // NOLINT

} // namespace psr

#endif
