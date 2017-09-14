/*
 * PluginTest.cpp
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#include "IFDSTabulationProblemTestPlugin.hh"

unique_ptr<IFDSTabulationProblemPlugin> createIFDSTabulationProblemPlugin(LLVMBasedICFG& I) {
	return unique_ptr<IFDSTabulationProblemPlugin>(new IFDSTabulationProblemTestPlugin(I));
}

IFDSTabulationProblemTestPlugin::IFDSTabulationProblemTestPlugin(LLVMBasedICFG& I) : IFDSTabulationProblemPlugin(I) {}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(const llvm::Instruction *curr,
                                                const llvm::Instruction *succ) {
  cout << "IFDSSolverTest::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                             const llvm::Function *destMthd) {
	cout << "IFDSSolverTest::getCallFlowFuntion()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getRetFlowFunction(const llvm::Instruction *callSite,
                                             const llvm::Function *calleeMthd,
                                             const llvm::Instruction *exitStmt,
                                             const llvm::Instruction *retSite) {
	cout << "IFDSSolverTest::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite) {
	cout << "IFDSSolverTest::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(const llvm::Instruction *callStmt,
											 	 	 	 	 	 	 	 	 	 	 	 	 	 const llvm::Function *destMthd,
																								 vector<const llvm::Value*> inputs,
																								 vector<bool> context) {
	cout << "IFDSSolverTest::getSummaryFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTabulationProblemTestPlugin::initialSeeds() {
//	cout << "IFDSSolverTest::initialSeeds()\n";
//  const llvm::Function *mainfunction = IFDSTabulationProblemPlugin::icfg.getModule().getFunction("main");
//  const llvm::Instruction *firstinst = &mainfunction->front().front();
//  set<const llvm::Value *> iset{zeroValue()};
//  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
//      {firstinst, iset}};
//  return imap;
	return {};
}

string IFDSTabulationProblemTestPlugin::D_to_string(const llvm::Value *d) {
  return llvmIRToString(d);
}
