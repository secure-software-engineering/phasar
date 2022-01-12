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

struct IFDSPluginAnalysisDomain : public LLVMIFDSAnalysisDomainDefault {
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
                                                std::move(EntryPoints)) {}
  ~IFDSTabulationProblemPlugin() override = default;

  bool isZeroValue(d_t Fact) const override { return Fact == getZeroValue(); }

  void printNode(std::ostream &OS, n_t Stmt) const override {
    OS << llvmIRToString(Stmt);
  }

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override {
    // os << llvmIRToString(d);
    Fact->print(OS);
  }

  void printFunction(std::ostream &OS, f_t Func) const override {
    OS << Func->getName().str();
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
