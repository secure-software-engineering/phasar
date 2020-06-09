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

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include <map>
#include <memory>
#include <string>

namespace psr {

class IntraMonoProblemPlugin
    : public IntraMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *,
                              const llvm::Value *, LLVMBasedCFG> {
public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedCFG;

  IntraMonoProblemPlugin(const ProjectIRDB *IRDB,
                         const TypeHierarchy<t_t, f_t> *TH, const i_t *CF,
                         const PointsToInfo<v_t, n_t> *PT,
                         std::set<std::string> EntryPoints)
      : IntraMonoProblem<n_t, d_t, f_t, t_t, v_t, i_t>(IRDB, TH, CF, PT,
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

extern std::unique_ptr<IntraMonoProblemPlugin> makeIntraMonoProblemPlugin(
    const ProjectIRDB *IRDB,
    const TypeHierarchy<IntraMonoProblemPlugin::t_t,
                        IntraMonoProblemPlugin::f_t> *TH,
    const IntraMonoProblemPlugin::i_t *CF,
    const PointsToInfo<IntraMonoProblemPlugin::v_t, IntraMonoProblemPlugin::n_t>
        *PT,
    std::set<std::string> EntryPoints);

extern std::map<std::string,
                std::unique_ptr<IntraMonoProblemPlugin> (*)(
                    const ProjectIRDB *IRDB,
                    const TypeHierarchy<IntraMonoProblemPlugin::t_t,
                                        IntraMonoProblemPlugin::f_t> *TH,
                    const IntraMonoProblemPlugin::i_t *CF,
                    const PointsToInfo<IntraMonoProblemPlugin::v_t,
                                       IntraMonoProblemPlugin::n_t> *PT,
                    std::set<std::string> EntryPoints)>
    IntraMonoProblemPluginFactory;

} // namespace psr

#endif
