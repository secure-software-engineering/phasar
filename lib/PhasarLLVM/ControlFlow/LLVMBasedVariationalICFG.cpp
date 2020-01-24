/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalICFG.h>

using namespace psr;

namespace psr {

LLVMBasedVariationalICFG::LLVMBasedVariationalICFG(
    ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    const std::set<std::string> &EntryPoints, LLVMTypeHierarchy *TH,
    LLVMPointsToInfo *PT)
    : LLVMBasedICFG(IRDB, CGType, EntryPoints, TH, PT) {}

} // namespace psr
