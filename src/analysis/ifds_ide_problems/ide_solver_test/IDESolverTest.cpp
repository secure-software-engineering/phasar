/*
 * IDESolverTest.cpp
 *
 *  Created on: 31.05.2017
 *      Author: philipp
 */

#include "IDESolverTest.hh"

IDESolverTest::IDESolverTest(LLVMBasedICFG &icfg, vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getNormalFlowFunction(const llvm::Instruction *curr,
                                     const llvm::Instruction *succ) {
  cout << "IDESolverTest::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getCallFlowFuntion(const llvm::Instruction *callStmt,
                                  const llvm::Function *destMthd) {
  cout << "IDESolverTest::getCallFlowFuntion()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>> IDESolverTest::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "IDESolverTest::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                        const llvm::Instruction *retSite) {
  cout << "IDESolverTest::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDESolverTest::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd,
                                      vector<const llvm::Value *> inputs,
                                      vector<bool> context) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDESolverTest::initialSeeds() {
  cout << "IDESolverTest::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IDESolverTest::createZeroValue() {
  cout << "IDESolverTest::createZeroValue()\n";
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

bool IDESolverTest::isZeroValue(const llvm::Value* d) {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getNormalEdgeFunction(const llvm::Instruction *curr,
                                     const llvm::Value *currNode,
                                     const llvm::Instruction *succ,
                                     const llvm::Value *succNode) {
  cout << "IDESolverTest::getNormalEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getCallEdgeFunction(const llvm::Instruction *callStmt,
                                   const llvm::Value *srcNode,
                                   const llvm::Function *destiantionMethod,
                                   const llvm::Value *destNode) {
  cout << "IDESolverTest::getCallEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getReturnEdgeFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMethod,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Value *exitNode,
                                     const llvm::Instruction *reSite,
                                     const llvm::Value *retNode) {
  cout << "IDESolverTest::getReturnEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getCallToReturnEdgeFunction(const llvm::Instruction *callSite,
                                           const llvm::Value *callNode,
                                           const llvm::Instruction *retSite,
                                           const llvm::Value *retSiteNode) {
  cout << "IDESolverTest::getCallToReturnEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::getSummaryEdgeFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd,
                                      vector<const llvm::Value *> inputs,
                                      vector<bool> context) {
  cout << "IDESolverTest::getSummaryEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

const llvm::Value *IDESolverTest::topElement() {
  cout << "IDESolverTest::topElement()\n";
  return nullptr;
}

const llvm::Value *IDESolverTest::bottomElement() {
  cout << "IDESolverTest::bottomElement()\n";
  return nullptr;
}

const llvm::Value *IDESolverTest::join(const llvm::Value *lhs,
                                       const llvm::Value *rhs) {
  cout << "IDESolverTest::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>> IDESolverTest::allTopFunction() {
  cout << "IDESolverTest::allTopFunction()\n";
  return make_shared<IDESolverTestAllTop>();
}

const llvm::Value *
IDESolverTest::IDESolverTestAllTop::computeTarget(const llvm::Value *source) {
  cout << "IDESolverTest::IDESolverTestAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> secondFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::composeWith()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> otherFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::joinWith()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

bool IDESolverTest::IDESolverTestAllTop::equalTo(
    shared_ptr<EdgeFunction<const llvm::Value *>> other) {
  cout << "IDESolverTest::IDESolverTestAllTop::equalTo()\n";
  return false;
}

string IDESolverTest::D_to_string(const llvm::Value *d) { return ""; }

string IDESolverTest::V_to_string(const llvm::Value *v) { return ""; }
