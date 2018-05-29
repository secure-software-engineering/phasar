/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSolverTest.cpp
 *
 *  Created on: 31.05.2017
 *      Author: philipp
 */

#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>
using namespace psr;
namespace psr{

IFDSSolverTest::IFDSSolverTest(LLVMBasedICFG &I, vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                   const llvm::Value *, const llvm::Function *,
                                   LLVMBasedICFG &>(I),
      EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSolverTest::getNormalFlowFunction(const llvm::Instruction *curr,
                                      const llvm::Instruction *succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getNormalFlowFunction()";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSolverTest::getCallFlowFunction(const llvm::Instruction *callStmt,
                                    const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getCallFlowFunction()";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSolverTest::getRetFlowFunction(const llvm::Instruction *callSite,
                                   const llvm::Function *calleeMthd,
                                   const llvm::Instruction *exitStmt,
                                   const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getRetFlowFunction()";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSolverTest::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                         const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getCallToRetFlowFunction()";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSSolverTest::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSSolverTest::initialSeeds() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::initialSeeds()";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSSolverTest::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSolverTest::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

string IFDSSolverTest::DtoString(const llvm::Value *d) const {
  return llvmIRToString(d);
}

string IFDSSolverTest::NtoString(const llvm::Instruction *n) const {
  return llvmIRToString(n);
}

string IFDSSolverTest::MtoString(const llvm::Function *m) const {
  return m->getName().str();
}
}//namespace psr