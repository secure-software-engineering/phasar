/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PluginTest.h
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_H_
#define SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_H_

#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h"

namespace psr {

struct ValueFlowFactWrapper : public FlowFactWrapper<const llvm::Value *> {

  using FlowFactWrapper::FlowFactWrapper;
  void print(llvm::raw_ostream &OS,
             const llvm::Value *const &NonzeroFact) const override {
    OS << llvmIRToShortString(NonzeroFact) << '\n';
  }
};

class IFDSTabulationProblemTestPlugin : public IFDSTabulationProblemPlugin {
  FlowFactManager<ValueFlowFactWrapper> FFManager;

public:
  IFDSTabulationProblemTestPlugin(const ProjectIRDB *IRDB,
                                  const LLVMTypeHierarchy *TH,
                                  const LLVMBasedICFG *ICF,
                                  LLVMPointsToInfo *PT,
                                  std::set<std::string> EntryPoints);

  ~IFDSTabulationProblemTestPlugin() override = default;

  [[nodiscard]] const FlowFact *createZeroValue() const override;

  FlowFunctionPtrType
  getNormalFlowFunction(const llvm::Instruction *Curr,
                        const llvm::Instruction *Succ) override;

  FlowFunctionPtrType
  getCallFlowFunction(const llvm::Instruction *CallSite,
                      const llvm::Function *DestFun) override;

  FlowFunctionPtrType
  getRetFlowFunction(const llvm::Instruction *CallSite,
                     const llvm::Function *CalleeFun,
                     const llvm::Instruction *ExitStmt,
                     const llvm::Instruction *RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(const llvm::Instruction *CallSite,
                           const llvm::Instruction *RetSite,
                           std::set<const llvm::Function *> Callees) override;

  FlowFunctionPtrType
  getSummaryFlowFunction(const llvm::Instruction *CallSite,
                         const llvm::Function *DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;
};

extern "C" std::unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSTabulationProblemTestPlugin(const ProjectIRDB *IRDB,
                                    const LLVMTypeHierarchy *TH,
                                    const LLVMBasedICFG *ICF,
                                    const LLVMPointsToInfo *PT,
                                    std::set<std::string> EntryPoints);

} // namespace psr

#endif
