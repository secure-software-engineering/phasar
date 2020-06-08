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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
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

class IDETabulationProblemPlugin
    : public IDETabulationProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::StructType *, const llvm::Value *,
                                  const llvm::Value *, LLVMBasedICFG> {
public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using l_t = const llvm::Value *;
  using i_t = LLVMBasedICFG;

protected:
  std::vector<std::string> EntryPoints;

public:
  IDETabulationProblemPlugin(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF,
                             const LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
      : IDETabulationProblem<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Function *, const llvm::StructType *,
                             const llvm::Value *, const llvm::Value *,
                             LLVMBasedICFG>(IRDB, TH, ICF, PT, EntryPoints) {
    ZeroValue = createZeroValue();
  }
  ~IDETabulationProblemPlugin() override = default;

  const llvm::Value *createZeroValue() const override {
    // create a special value to represent the zero value!
    return LLVMZeroValue::getInstance();
  }

  bool isZeroValue(const llvm::Value *d) const override {
    return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
  }

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os,
                         const llvm::Value *d) const override {
    os << llvmIRToString(d);
  }

  void printFunction(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }

  void printEdgeFact(std::ostream &os, const llvm::Value *l) const override {
    os << llvmIRToString(l);
  }

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    return std::make_shared<AllTop<l_t>>(topElement());
  }
};

/*extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemPlugin(const ProjectIRDB *IRDB,
                               const LLVMTypeHierarchy *TH,
                               const LLVMBasedICFG *ICF,
                               const LLVMPointsToInfo *PT,
                               std::set<std::string> EntryPoints);*/

extern std::map<std::string,
                std::unique_ptr<IDETabulationProblemPlugin> (*)(
                    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    IDETabulationProblemPluginFactory;

} // namespace psr

#endif
