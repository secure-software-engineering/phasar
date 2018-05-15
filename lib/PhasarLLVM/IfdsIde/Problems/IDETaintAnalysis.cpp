/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETaintAnalysis.h>

bool IDETaintAnalysis::set_contains_str(set<string> s, string str) {
  return s.find(str) != s.end();
}

IDETaintAnalysis::IDETaintAnalysis(LLVMBasedICFG &icfg,
                                   vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                        const llvm::Instruction *succ) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                      const llvm::Function *destMthd) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                     const llvm::Function *calleeMthd,
                                     const llvm::Instruction *exitStmt,
                                     const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                           const llvm::Instruction *retSite) {
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IDETaintAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                         const llvm::Function *destMthd) {
  return nullptr;
}

map<const llvm::Instruction *, set<const llvm::Value *>>
IDETaintAnalysis::initialSeeds() {
  // just start in main()
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IDETaintAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETaintAnalysis::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getNormalEdgeFunction(const llvm::Instruction *curr,
                                        const llvm::Value *currNode,
                                        const llvm::Instruction *succ,
                                        const llvm::Value *succNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getCallEdgeFunction(const llvm::Instruction *callStmt,
                                      const llvm::Value *srcNode,
                                      const llvm::Function *destiantionMethod,
                                      const llvm::Value *destNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getReturnEdgeFunction(const llvm::Instruction *callSite,
                                        const llvm::Function *calleeMethod,
                                        const llvm::Instruction *exitStmt,
                                        const llvm::Value *exitNode,
                                        const llvm::Instruction *reSite,
                                        const llvm::Value *retNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getCallToReturnEdgeFunction(const llvm::Instruction *callSite,
                                              const llvm::Value *callNode,
                                              const llvm::Instruction *retSite,
                                              const llvm::Value *retSiteNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::getSummaryEdgeFunction(const llvm::Instruction *callStmt,
                                         const llvm::Value *callNode,
                                         const llvm::Instruction *retSite,
                                         const llvm::Value *retSiteNode) {
  return EdgeIdentity<const llvm::Value *>::v();
}

const llvm::Value *IDETaintAnalysis::topElement() { return nullptr; }

const llvm::Value *IDETaintAnalysis::bottomElement() { return nullptr; }

const llvm::Value *IDETaintAnalysis::join(const llvm::Value *lhs,
                                          const llvm::Value *rhs) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::allTopFunction() {
  return make_shared<IDETainAnalysisAllTop>();
}

const llvm::Value *IDETaintAnalysis::IDETainAnalysisAllTop::computeTarget(
    const llvm::Value *source) {
  return nullptr;
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::IDETainAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> secondFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

shared_ptr<EdgeFunction<const llvm::Value *>>
IDETaintAnalysis::IDETainAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<const llvm::Value *>> otherFunction) {
  return EdgeIdentity<const llvm::Value *>::v();
}

bool IDETaintAnalysis::IDETainAnalysisAllTop::equalTo(
    shared_ptr<EdgeFunction<const llvm::Value *>> other) {
  return false;
}

string IDETaintAnalysis::DtoString(const llvm::Value *d) const {
  return llvmIRToString(d);
}

string IDETaintAnalysis::VtoString(const llvm::Value *v) const {
  return llvmIRToString(v);
}

string IDETaintAnalysis::NtoString(const llvm::Instruction *n) const {
  return llvmIRToString(n);
}

string IDETaintAnalysis::MtoString(const llvm::Function *m) const {
  return m->getName().str();
}