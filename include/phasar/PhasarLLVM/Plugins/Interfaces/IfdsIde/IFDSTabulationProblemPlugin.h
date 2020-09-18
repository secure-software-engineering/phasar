/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSTabulationProblemPlugin.h
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFactWrapper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

class ProjectIRDB;

struct IFDSPluginAnalysisDomain : public LLVMAnalysisDomainDefault {
  using d_t = const FlowFact *;
};

class IFDSTabulationProblemPlugin
    : public IFDSTabulationProblem<IFDSPluginAnalysisDomain> {
  using AnalysisDomainTy = IFDSPluginAnalysisDomain;

public:
  IFDSTabulationProblemPlugin(const ProjectIRDB *IRDB,
                              const LLVMTypeHierarchy *TH,
                              const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                              std::set<std::string> EntryPoints)
      : IFDSTabulationProblem<AnalysisDomainTy>(IRDB, TH, ICF, PT,
                                                EntryPoints) {}
  ~IFDSTabulationProblemPlugin() override = default;

  bool isZeroValue(d_t d) const override { return d == getZeroValue(); }

  void printNode(std::ostream &os, n_t n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    // os << llvmIRToString(d);
    d->print(os);
  }

  void printFunction(std::ostream &os, f_t m) const override {
    os << m->getName().str();
  }
};

extern std::map<std::string,
                std::unique_ptr<IFDSTabulationProblemPlugin> (*)(
                    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    IFDSTabulationProblemPluginFactory;

} // namespace psr

#endif
