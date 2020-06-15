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
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

namespace psr {

class InterMonoProblemPlugin
    : public InterMonoProblem<LLVMAnalysisDomainDefault> {
public:
  InterMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                         std::set<std::string> EntryPoints)
      : InterMonoProblem(IRDB, TH, ICF, PT, EntryPoints) {}

  void printNode(std::ostream &os, n_t n) const override {
    os << llvmIRToString(n);
  }
  void printDataFlowFact(std::ostream &os, d_t d) const override {
    os << llvmIRToString(d);
  }
  void printFunction(std::ostream &os, f_t f) const override {
    os << f->getName().str();
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
