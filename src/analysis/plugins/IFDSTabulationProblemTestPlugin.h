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

#include "../ifds_ide/flow_func/Gen.h"
#include "plugin_ifaces/ifds_ide/IFDSTabulationProblemPlugin.h"

class IFDSTabulationProblemTestPlugin : public IFDSTabulationProblemPlugin {
public:
  IFDSTabulationProblemTestPlugin(LLVMBasedICFG &I, vector<string> EntryPoints);
  ~IFDSTabulationProblemTestPlugin() = default;
  shared_ptr<FlowFunction<const llvm::Value *>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destMthd) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destMthd) override;

  map<const llvm::Instruction *, set<const llvm::Value *>>
  initialSeeds() override;
};

extern "C" unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSTabulationProblemTestPlugin(LLVMBasedICFG &I,
                                    vector<string> EntryPoints);

#endif /* SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_HH_ */
