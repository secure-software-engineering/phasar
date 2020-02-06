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

#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>

namespace psr {

class IFDSSimpleTaintAnalysis : public IFDSTabulationProblemPlugin {
public:
  IFDSSimpleTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                          const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                          std::set<std::string> EntryPoints = {});
  ~IFDSSimpleTaintAnalysis() = default;
  std::shared_ptr<FlowFunction<const llvm::Value *>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  std::shared_ptr<FlowFunction<const llvm::Value *>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destFun) override;

  std::shared_ptr<FlowFunction<const llvm::Value *>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeFun,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  std::shared_ptr<FlowFunction<const llvm::Value *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  std::shared_ptr<FlowFunction<const llvm::Value *>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destFun) override;

  std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
  initialSeeds() override;
};

extern "C" std::unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSimpleTaintAnalysis(LLVMBasedICFG &I,
                            std::vector<std::string> EntryPoints);
} // namespace psr

#endif /* SRC_ANALYSIS_PLUGINS_IFDSSIMPLETAINTANALYSIS_H_ */
