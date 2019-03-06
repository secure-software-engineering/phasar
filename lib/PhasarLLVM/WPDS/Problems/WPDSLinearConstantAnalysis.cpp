/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSLinearConstantAnalysis.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSLinearConstantAnalysis::WPDSLinearConstantAnalysis(
    LLVMBasedICFG &I, const LLVMTypeHierarchy &TH, const ProjectIRDB &DB,
    WPDSType WPDS, SearchDirection Direction,
    std::vector<std::string> EntryPoints, std::vector<n_t> Stack,
    bool Witnesses)
    : LLVMDefaultWPDSProblem(I, TH, DB, WPDS, Direction, Stack, Witnesses),
      IDELinearConstantAnalysis(I, EntryPoints) {}

LLVMBasedICFG &WPDSLinearConstantAnalysis::interproceduralCFG() {
  return this->icfg;
}

} // namespace psr
