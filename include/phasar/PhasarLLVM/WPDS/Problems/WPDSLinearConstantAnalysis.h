/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H_
#define PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H_

#include <string>
#include <vector>

#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>
#include <phasar/PhasarLLVM/WPDS/LLVMDefaultWPDSProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class ProjectIRDB;

class WPDSLinearConstantAnalysis
    : public LLVMDefaultWPDSProblem<const llvm::Value *, int64_t, LLVMBasedICFG &>, public IDELinearConstantAnalysis {

public:
  WPDSLinearConstantAnalysis(LLVMBasedICFG &I, const LLVMTypeHierarchy &TH, const ProjectIRDB &DB, WPDSType WPDS,
                             SearchDirection Direction,
                             std::vector<std::string> EntryPoints = {"main"},
                             std::vector<n_t> Stack = {},
                             bool Witnesses = false);

  ~WPDSLinearConstantAnalysis() override = default;

  LLVMBasedICFG &interproceduralCFG() override;
};

} // namespace psr

#endif
