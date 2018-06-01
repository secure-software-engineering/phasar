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

#ifndef SRC_ANALYSIS_PLUGINS_IFDSSIMPLETAINTANALYSIS_H_
#define SRC_ANALYSIS_PLUGINS_IFDSSIMPLETAINTANALYSIS_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>

class IFDSSimpleTaintAnalysis : public IFDSTabulationProblemPlugin {
public:
  IFDSSimpleTaintAnalysis(LLVMBasedICFG &I, vector<string> EntryPoints);
  ~IFDSSimpleTaintAnalysis() = default;
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
                           const llvm::Instruction *retSite,
                           set<const llvm::Function *> callees) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destMthd) override;

  map<const llvm::Instruction *, set<const llvm::Value *>>
  initialSeeds() override;
};

extern "C" unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSimpleTaintAnalysis(LLVMBasedICFG &I, vector<string> EntryPoints);

#endif /* SRC_ANALYSIS_PLUGINS_IFDSSIMPLETAINTANALYSIS_H_ */
