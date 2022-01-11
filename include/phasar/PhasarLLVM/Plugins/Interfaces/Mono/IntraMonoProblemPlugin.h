/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTRAMONOPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTRAMONOPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

struct IntraMonoProblemPluginDomain : LLVMAnalysisDomainDefault {
  using mono_container_t = std::set<LLVMAnalysisDomainDefault::d_t>;
};

class IntraMonoProblemPlugin
    : public IntraMonoProblem<IntraMonoProblemPluginDomain> {
public:
  IntraMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedCFG *CF, LLVMPointsToInfo *PT,
                         std::set<std::string> EntryPoints)
      : IntraMonoProblem(IRDB, TH, CF, PT, std::move(EntryPoints)) {}

  void printNode(std::ostream &OS, n_t Stmt) const override {
    OS << llvmIRToString((llvm::Value *)Stmt);
  }
  void printDataFlowFact(std::ostream &OS, d_t Fact) const override {
    OS << llvmIRToString(Fact);
  }
  void printFunction(std::ostream &OS, f_t Func) const override {
    OS << Func->getName().str();
  }
};

extern std::unique_ptr<IntraMonoProblemPlugin>
makeIntraMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedCFG *CF, LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints);

extern std::map<std::string,
                std::unique_ptr<IntraMonoProblemPlugin> (*)(
                    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedCFG *CF, LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    IntraMonoProblemPluginFactory;

} // namespace psr

#endif
