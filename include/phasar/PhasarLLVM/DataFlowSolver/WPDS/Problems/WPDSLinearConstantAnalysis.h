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

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSProblem.h>

namespace llvm {
class Value;
class Instruction;
class StructType;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMPointsToInfo;
class LLVMTypeHierarchy;
class ProjectIRDB;

class WPDSLinearConstantAnalysis
    : public virtual WPDSProblem<const llvm::Instruction *, const llvm::Value *,
                                 const llvm::Function *,
                                 const llvm::StructType *, const llvm::Value *,
                                 int64_t, LLVMBasedICFG>,
      public virtual IDELinearConstantAnalysis {
public:
  WPDSLinearConstantAnalysis(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF,
                             const LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints = {"main"});

  ~WPDSLinearConstantAnalysis() override = default;
};

} // namespace psr

#endif
