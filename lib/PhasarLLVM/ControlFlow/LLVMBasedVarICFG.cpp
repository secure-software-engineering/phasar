/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"

using namespace psr;

namespace psr {

LLVMBasedVarICFG::LLVMBasedVarICFG(
    ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    const std::set<std::string> &EntryPoints, LLVMTypeHierarchy *TH,
    LLVMPointsToInfo *PT, const stringstringmap_t *StaticBackwardRenaming)
    : LLVMBasedICFG(IRDB, CGType, EntryPoints, TH, PT),
      LLVMBasedVarCFG(IRDB, StaticBackwardRenaming) {}

} // namespace psr
