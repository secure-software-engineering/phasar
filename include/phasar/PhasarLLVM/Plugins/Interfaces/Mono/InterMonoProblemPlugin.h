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

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include <map>
#include <memory>
#include <string>

namespace psr {

class InterMonoProblemPlugin
    : public InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *,
                              const llvm::Value *, LLVMBasedICFG> {
public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedICFG;

  InterMonoProblemPlugin(const ProjectIRDB *IRDB,
                         const TypeHierarchy<t_t, f_t> *TH, const i_t *ICF,
                         const PointsToInfo<v_t, n_t> *PT,
                         std::set<std::string> EntryPoints)
      : InterMonoProblem<n_t, d_t, f_t, t_t, v_t, i_t>(IRDB, TH, ICF, PT,
                                                       EntryPoints) {}

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
                    const ProjectIRDB *IRDB,
                    const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF,
                    LLVMPointsToInfo *PT,
                    std::set<std::string> EntryPoints)>
    InterMonoProblemPluginFactory;

} // namespace psr

#endif
