/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTERMONOPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTERMONOPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

struct InterMonoProblemPluginDomain : LLVMAnalysisDomainDefault {
  using mono_container_t = std::set<LLVMAnalysisDomainDefault::d_t>;
};

class InterMonoProblemPlugin
    : public InterMonoProblem<InterMonoProblemPluginDomain> {
public:
  InterMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                         std::set<std::string> EntryPoints)
      : InterMonoProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

  void printNode(std::ostream &OS, n_t Inst) const override {
    OS << llvmIRToString((llvm::Value *)Inst);
  }
  void printDataFlowFact(std::ostream &OS, d_t Fact) const override {
    OS << llvmIRToString(Fact);
  }
  void printFunction(std::ostream &OS, f_t Fun) const override {
    OS << Fun->getName().str();
  }
};

extern std::unique_ptr<InterMonoProblemPlugin>
makeInterMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints);

extern std::map<std::string,
                std::unique_ptr<InterMonoProblemPlugin> (*)(
                    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    InterMonoProblemPluginFactory;

} // namespace psr

#endif
