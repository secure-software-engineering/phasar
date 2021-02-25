/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

using namespace std;
using namespace psr;

namespace psr {

WPDSLinearConstantAnalysis::WPDSLinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    const std::set<std::string> &EntryPoints)
    : WPDSProblem<WPDSLinearConstantAnalysisDomain>(IRDB, TH, ICF, PT,
                                                    EntryPoints),
      IDELinearConstantAnalysis(IRDB, TH, ICF, PT, EntryPoints) {
  WPDSProblem<WPDSLinearConstantAnalysisDomain>::ZeroValue =
      IDELinearConstantAnalysis::createZeroValue();
}

} // namespace psr
