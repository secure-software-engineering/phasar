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
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

namespace psr {

class IntraMonoProblemPlugin
    : public IntraMonoProblem<LLVMAnalysisDomainDefault> {
public:
  IntraMonoProblemPlugin(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedCFG *CF, LLVMPointsToInfo *PT,
                         std::set<std::string> EntryPoints)
      : IntraMonoProblem(IRDB, TH, CF, PT, EntryPoints) {}

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
