/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSignAnalysis.cpp
 *
 *  Created on: 26.01.2018
 *      Author: philipp
 */

#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSignAnalysis.h>

IFDSSignAnalysis::IFDSSignAnalysis(LLVMBasedICFG &I, vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                   const llvm::Value *, const llvm::Function *,
                                   LLVMBasedICFG &>(I),
      EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSignAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                        const llvm::Instruction *succ) {
  cout << "IFDSSignAnalysis::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSignAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  cout << "IFDSSignAnalysis::getCallFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSignAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMthd,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Instruction *retSite) {
  cout << "IFDSSignAnalysis::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSignAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  cout << "IFDSSignAnalysis::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSignAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                         const llvm::Function *destMthd) {
  cout << "IFDSSignAnalysis::getSummaryFlowFunction()\n";
  return Identity<const llvm::Value *>::getInstance();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSSignAnalysis::initialSeeds() {
  cout << "IFDSSignAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSSignAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSignAnalysis::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

string IFDSSignAnalysis::DtoString(const llvm::Value *d) const {
  return llvmIRToString(d);
}

string IFDSSignAnalysis::NtoString(const llvm::Instruction *n) const {
  return llvmIRToString(n);
}

string IFDSSignAnalysis::MtoString(const llvm::Function *m) const {
  return m->getName().str();
}
