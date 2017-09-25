/*
 * PluginTest.cpp
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#include "IFDSTabulationProblemTestPlugin.hh"

unique_ptr<IFDSTabulationProblemPlugin>
createIFDSTabulationProblemPlugin(LLVMBasedICFG &I,
                                  vector<string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSTabulationProblemTestPlugin(I, EntryPoints));
}

IFDSTabulationProblemTestPlugin::IFDSTabulationProblemTestPlugin(
    LLVMBasedICFG &I, vector<string> EntryPoints)
    : IFDSTabulationProblemPlugin(I, EntryPoints) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *curr, const llvm::Instruction *succ) {
  cout << "IFDSTabulationProblemTestPlugin::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IFDSTabulationProblemTestPlugin::getCallFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "IFDSTabulationProblemTestPlugin::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite) {
  cout << "IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd,
    vector<const llvm::Value *> inputs, vector<bool> context) {
  cout << "IFDSTabulationProblemTestPlugin::getSummaryFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::initialSeeds() {
  cout << "IFDSTabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}
