/*
 * IDESolverTest.cpp
 *
 *  Created on: 31.05.2017
 *      Author: philipp
 */

#include "IDESolverTest.hh"

IDESolverTest::IDESolverTest(LLVMBasedICFG &icfg)
    : DefaultIDETabulationProblem(icfg) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getNormalFlowFunction(const llvm::Instruction *curr,
                                     const llvm::Instruction *succ) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                  const llvm::Function *destMthd) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>> IDESolverTest::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                        const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDESolverTest::initialSeeds() {
  // just start in main()
  const llvm::Function *mainfunction = icfg.getModule().getFunction("main");
  const llvm::Instruction *firstinst = &(*(mainfunction->begin()->begin()));
  set<const llvm::Value *> iset{zeroValue()};
  map<const llvm::Instruction *, set<const llvm::Value *>> imap{
      {firstinst, iset}};
  return imap;
}

const llvm::Value *IDESolverTest::createZeroValue() {
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getNormalEdgeFunction(const llvm::Instruction *curr,
                                     const llvm::Value *currNode,
                                     const llvm::Instruction *succ,
                                     const llvm::Value *succNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getCallEdgeFunction(const llvm::Instruction *callStmt,
                                   const llvm::Value *srcNode,
                                   const llvm::Function *destiantionMethod,
                                   const llvm::Value *destNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getReturnEdgeFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMethod,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Value *exitNode,
                                     const llvm::Instruction *reSite,
                                     const llvm::Value *retNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getCallToReturnEdgeFunction(const llvm::Instruction *callSite,
                                           const llvm::Value *callNode,
                                           const llvm::Instruction *retSite,
                                           const llvm::Value *retSiteNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

const llvm::Value *IDESolverTest::topElement() { return nullptr; }

const llvm::Value *IDESolverTest::bottomElement() { return nullptr; }

const llvm::Value *IDESolverTest::join(const llvm::Value *lhs,
                                       const llvm::Value *rhs) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>> IDESolverTest::allTopFunction() {
  return make_shared<IDESolverTestAllTop>();
}

const llvm::Value *
IDESolverTest::IDESolverTestAllTop::computeTarget(const llvm::Value *source) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> secondFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> otherFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

bool IDESolverTest::IDESolverTestAllTop::equalTo(
    shared_ptr<EdgeFunction<const llvm::Value *>> other) {
  return false;
}
