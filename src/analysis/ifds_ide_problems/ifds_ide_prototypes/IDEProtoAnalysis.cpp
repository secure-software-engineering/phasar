/*
 * IDEProtoAnalysis.cpp
 *
 *  Created on: 15.09.2017
 *      Author: philipp
 */

#include "IDEProtoAnalysis.hh"

IDEProtoAnalysis::IDEProtoAnalysis(LLVMBasedICFG &icfg,
                                   vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<const llvm::Value *>>
IDEProtoAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                        const llvm::Instruction *succ) {
  cout << "IDEProtoAnalysis::getNormalFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDEProtoAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                     const llvm::Function *destMthd) {
  cout << "IDEProtoAnalysis::getCallFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDEProtoAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMthd,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Instruction *retSite) {
  cout << "IDEProtoAnalysis::getRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDEProtoAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                           const llvm::Instruction *retSite) {
  cout << "IDEProtoAnalysis::getCallToRetFlowFunction()\n";
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDEProtoAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                         const llvm::Function *destMthd,
                                         vector<const llvm::Value *> inputs,
                                         vector<bool> context) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDEProtoAnalysis::initialSeeds() {
  cout << "IDEProtoAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IDEProtoAnalysis::createZeroValue() {
  cout << "IDEProtoAnalysis::createZeroValue()\n";
  // create a special value to represent the zero value!
  static ZeroValue *zero = new ZeroValue;
  return zero;
}

bool IDEProtoAnalysis::isZeroValue(const llvm::Value* d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::getNormalEdgeFunction(const llvm::Instruction *curr,
                                        const llvm::Value *currNode,
                                        const llvm::Instruction *succ,
                                        const llvm::Value *succNode) {
  cout << "IDEProtoAnalysis::getNormalEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::getCallEdgeFunction(const llvm::Instruction *callStmt,
                                      const llvm::Value *srcNode,
                                      const llvm::Function *destiantionMethod,
                                      const llvm::Value *destNode) {
  cout << "IDEProtoAnalysis::getCallEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::getReturnEdgeFunction(const llvm::Instruction *callSite,
                                        const llvm::Function *calleeMethod,
                                        const llvm::Instruction *exitStmt,
                                        const llvm::Value *exitNode,
                                        const llvm::Instruction *reSite,
                                        const llvm::Value *retNode) {
  cout << "IDEProtoAnalysis::getReturnEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::getCallToReturnEdgeFunction(const llvm::Instruction *callSite,
                                              const llvm::Value *callNode,
                                              const llvm::Instruction *retSite,
                                              const llvm::Value *retSiteNode) {
  cout << "IDEProtoAnalysis::getCallToReturnEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::getSummaryEdgeFunction(const llvm::Instruction *callStmt,
                                         const llvm::Function *destMthd,
                                         vector<const llvm::Value *> inputs,
                                         vector<bool> context) {
  cout << "IDEProtoAnalysis::getSummaryEdgeFunction()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

const llvm::Value *IDEProtoAnalysis::topElement() {
  cout << "IDEProtoAnalysis::topElement()\n";
  return nullptr;
}

const llvm::Value *IDEProtoAnalysis::bottomElement() {
  cout << "IDEProtoAnalysis::bottomElement()\n";
  return nullptr;
}

const llvm::Value *IDEProtoAnalysis::join(const llvm::Value *lhs,
                                          const llvm::Value *rhs) {
  cout << "IDEProtoAnalysis::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::allTopFunction() {
  cout << "IDEProtoAnalysis::allTopFunction()\n";
  return make_shared<IDEProtoAnalysisAllTop>();
}

const llvm::Value *IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget(
    const llvm::Value *source) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> secondFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> otherFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith()\n";
  return EdgeIdentity<const llvm::Value *>::v();
}

bool IDEProtoAnalysis::IDEProtoAnalysisAllTop::equalTo(
    shared_ptr<EdgeFunction<const llvm::Value *>> other) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::equalTo()\n";
  return false;
}

string IDEProtoAnalysis::D_to_string(const llvm::Value *d) {
  return llvmIRToString(d);
}

string IDEProtoAnalysis::V_to_string(const llvm::Value *v) {
  return llvmIRToString(v);
}

string IDEProtoAnalysis::N_to_string(const llvm::Instruction *n) {
  return llvmIRToString(n);
}

string IDEProtoAnalysis::M_to_string(const llvm::Function *m) {
  return m->getName().str();
}
