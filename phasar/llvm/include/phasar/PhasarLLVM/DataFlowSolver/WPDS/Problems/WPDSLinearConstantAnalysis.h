/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H

#include <string>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

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

struct WPDSLinearConstantAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = int64_t;
};

class WPDSLinearConstantAnalysis
    : public virtual WPDSProblem<WPDSLinearConstantAnalysisDomain>,
      public virtual IDELinearConstantAnalysis {
public:
  WPDSLinearConstantAnalysis(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             const std::set<std::string> &EntryPoints = {
                                 "main"});

  ~WPDSLinearConstantAnalysis() override = default;
};

} // namespace psr

#endif
