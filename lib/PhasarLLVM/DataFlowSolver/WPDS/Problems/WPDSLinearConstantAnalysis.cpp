/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSLinearConstantAnalysis::WPDSLinearConstantAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : WPDSProblem<
          WPDSLinearConstantAnalysis::n_t, WPDSLinearConstantAnalysis::d_t,
          WPDSLinearConstantAnalysis::f_t, WPDSLinearConstantAnalysis::t_t,
          WPDSLinearConstantAnalysis::v_t, WPDSLinearConstantAnalysis::l_t,
          WPDSLinearConstantAnalysis::i_t>(IRDB, TH, ICF, PT, EntryPoints),
      IDELinearConstantAnalysis(IRDB, TH, ICF, PT, EntryPoints) {
  WPDSProblem<WPDSLinearConstantAnalysis::n_t, WPDSLinearConstantAnalysis::d_t,
              WPDSLinearConstantAnalysis::f_t, WPDSLinearConstantAnalysis::t_t,
              WPDSLinearConstantAnalysis::v_t, WPDSLinearConstantAnalysis::l_t,
              WPDSLinearConstantAnalysis::i_t>::ZeroValue =
      IDELinearConstantAnalysis::createZeroValue();
}

} // namespace psr
