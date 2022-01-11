/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFactWrapper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFactWrapper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
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

class LLVMBasedICFG;
class LLVMPointsToInfo;
class LLVMTypeHierarchy;
class ProjectIRDB;

struct IDEPluginAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = const EdgeFact *;
  using d_t = const FlowFact *;
};

class IDETabulationProblemPlugin
    : public IDETabulationProblem<IDEPluginAnalysisDomain> {
  using AnalysisDomainTy = IDEPluginAnalysisDomain;

public:
  IDETabulationProblemPlugin(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
      : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {}
  ~IDETabulationProblemPlugin() override = default;

  bool isZeroValue(d_t Fact) const override { return Fact == getZeroValue(); }

  void printNode(std::ostream &OS,
                 const llvm::Instruction *Stmt) const override {
    OS << llvmIRToString(Stmt);
  }

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override {
    // os << llvmIRToString(d);
    Fact->print(OS);
  }

  void printFunction(std::ostream &OS,
                     const llvm::Function *Func) const override {
    OS << Func->getName().str();
  }

  void printEdgeFact(std::ostream &OS, l_t L) const override {
    // os << llvmIRToString(l);
    L->print(OS);
  }

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    return std::make_shared<AllTop<l_t>>(topElement());
  }
};

extern std::map<std::string,
                std::unique_ptr<IDETabulationProblemPlugin> (*)(
                    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    IDETabulationProblemPluginFactory;

} // namespace psr

#endif
