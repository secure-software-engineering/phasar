/*
 * PluginTest.hh
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_HH_
#define SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_HH_

#include "plugin_ifaces/IFDSTabulationProblemPlugin.hh"

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

#endif /* SRC_ANALYSIS_PLUGINS_IFDSTABULATIONPROBLEMTESTPLUGIN_HH_ */
