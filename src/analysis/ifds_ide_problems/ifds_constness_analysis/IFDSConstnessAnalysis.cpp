/*
 * IFDSConstnessAnalysis.cpp
 *
 *  Created on: 07.06.2017
 *      Author: richard
 */

#include "IFDSConstnessAnalysis.hh"

IFDSConstnessAnalysis::IFDSConstnessAnalysis(LLVMBasedICFG &icfg)
    : DefaultIFDSTabulationProblem(icfg) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstnessAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                             const llvm::Instruction *succ) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getNormalFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstnessAnalysis::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                          const llvm::Function *destMthd) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstnessAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                          const llvm::Function *calleeMthd,
                                          const llvm::Instruction *exitStmt,
                                          const llvm::Instruction *retSite) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getRetFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstnessAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                                const llvm::Instruction *retSite) {
  cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallToRetFlowFunction()"
       << endl;
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSConstnessAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                              const llvm::Function *destMthd,
                                              vector<const llvm::Value *> inputs,
                                              vector<bool> context) {
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSConstnessAnalysis::initialSeeds() {
  const llvm::Function *mainfunction = icfg.getModule().getFunction("main");
  const llvm::Instruction *firstinst = &(*mainfunction->begin()->begin());
  set<const llvm::Value *> iset{zeroValue()};
  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
      {firstinst, iset}};
  return imap;
}

const llvm::Value *IFDSConstnessAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}
