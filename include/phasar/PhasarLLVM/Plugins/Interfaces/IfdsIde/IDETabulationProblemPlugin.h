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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ZeroFlowFact.h"
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

struct GeneralIDEAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = const EdgeFact *;
  using d_t = const FlowFact *;
};

class IDETabulationProblemPlugin
    : public IDETabulationProblem<GeneralIDEAnalysisDomain> {
  using AnalysisDomainTy = GeneralIDEAnalysisDomain;

public:
  IDETabulationProblemPlugin(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
      : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
    ZeroValue = createZeroValue();
  }
  ~IDETabulationProblemPlugin() override = default;

  d_t createZeroValue() const override {
    // create a special value to represent the zero value!
    // return LLVMZeroValue::getInstance();
    return ZeroFlowFact::getInstance();
  }

  bool isZeroValue(d_t d) const override {
    // return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
    return d == getZeroValue();
  }

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    // os << llvmIRToString(d);
    d->print(os);
  }

  void printFunction(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }

  void printEdgeFact(std::ostream &os, l_t l) const override {
    // os << llvmIRToString(l);
    l->print(os);
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
