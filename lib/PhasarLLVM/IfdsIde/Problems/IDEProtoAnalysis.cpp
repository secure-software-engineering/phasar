/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultSeeds.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDEProtoAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace psr;
using namespace std;

namespace psr {

IDEProtoAnalysis::IDEProtoAnalysis(IDEProtoAnalysis::i_t icfg,
                                   vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getNormalFlowFunction(IDEProtoAnalysis::n_t curr,
                                        IDEProtoAnalysis::n_t succ) {
  cout << "IDEProtoAnalysis::getNormalFlowFunction()\n";
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getCallFlowFunction(IDEProtoAnalysis::n_t callStmt,
                                      IDEProtoAnalysis::m_t destMthd) {
  cout << "IDEProtoAnalysis::getCallFlowFunction()\n";
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getRetFlowFunction(IDEProtoAnalysis::n_t callSite,
                                     IDEProtoAnalysis::m_t calleeMthd,
                                     IDEProtoAnalysis::n_t exitStmt,
                                     IDEProtoAnalysis::n_t retSite) {
  cout << "IDEProtoAnalysis::getRetFlowFunction()\n";
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getCallToRetFlowFunction(IDEProtoAnalysis::n_t callSite,
                                           IDEProtoAnalysis::n_t retSite,
                                           set<IDEProtoAnalysis::m_t> callees) {
  cout << "IDEProtoAnalysis::getCallToRetFlowFunction()\n";
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getSummaryFlowFunction(IDEProtoAnalysis::n_t callStmt,
                                         IDEProtoAnalysis::m_t destMthd) {
  return nullptr;
}

map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::initialSeeds() {
  cout << "IDEProtoAnalysis::initialSeeds()\n";
  map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IDEProtoAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDEProtoAnalysis::d_t IDEProtoAnalysis::createZeroValue() {
  cout << "IDEProtoAnalysis::createZeroValue()\n";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDEProtoAnalysis::isZeroValue(IDEProtoAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::getNormalEdgeFunction(IDEProtoAnalysis::n_t curr,
                                        IDEProtoAnalysis::d_t currNode,
                                        IDEProtoAnalysis::n_t succ,
                                        IDEProtoAnalysis::d_t succNode) {
  cout << "IDEProtoAnalysis::getNormalEdgeFunction()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::getCallEdgeFunction(IDEProtoAnalysis::n_t callStmt,
                                      IDEProtoAnalysis::d_t srcNode,
                                      IDEProtoAnalysis::m_t destiantionMethod,
                                      IDEProtoAnalysis::d_t destNode) {
  cout << "IDEProtoAnalysis::getCallEdgeFunction()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::getReturnEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                        IDEProtoAnalysis::m_t calleeMethod,
                                        IDEProtoAnalysis::n_t exitStmt,
                                        IDEProtoAnalysis::d_t exitNode,
                                        IDEProtoAnalysis::n_t reSite,
                                        IDEProtoAnalysis::d_t retNode) {
  cout << "IDEProtoAnalysis::getReturnEdgeFunction()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::getCallToRetEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                           IDEProtoAnalysis::d_t callNode,
                                           IDEProtoAnalysis::n_t retSite,
                                           IDEProtoAnalysis::d_t retSiteNode,
                                           set<IDEProtoAnalysis::m_t> callees) {
  cout << "IDEProtoAnalysis::getCallToRetEdgeFunction()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::getSummaryEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                         IDEProtoAnalysis::d_t callNode,
                                         IDEProtoAnalysis::n_t retSite,
                                         IDEProtoAnalysis::d_t retSiteNode) {
  cout << "IDEProtoAnalysis::getSummaryEdgeFunction()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

IDEProtoAnalysis::v_t IDEProtoAnalysis::topElement() {
  cout << "IDEProtoAnalysis::topElement()\n";
  return nullptr;
}

IDEProtoAnalysis::v_t IDEProtoAnalysis::bottomElement() {
  cout << "IDEProtoAnalysis::bottomElement()\n";
  return nullptr;
}

IDEProtoAnalysis::v_t IDEProtoAnalysis::join(IDEProtoAnalysis::v_t lhs,
                                             IDEProtoAnalysis::v_t rhs) {
  cout << "IDEProtoAnalysis::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::allTopFunction() {
  cout << "IDEProtoAnalysis::allTopFunction()\n";
  return make_shared<IDEProtoAnalysisAllTop>();
}

IDEProtoAnalysis::v_t IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget(
    IDEProtoAnalysis::v_t source) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>> secondFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>> otherFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::v_t>::getInstance();
}

bool IDEProtoAnalysis::IDEProtoAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::v_t>> other) const {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::equalTo()\n";
  return false;
}

void IDEProtoAnalysis::printNode(ostream &os, IDEProtoAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IDEProtoAnalysis::printDataFlowFact(ostream &os,
                                         IDEProtoAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IDEProtoAnalysis::printMethod(ostream &os, IDEProtoAnalysis::m_t m) const {
  os << m->getName().str();
}

void IDEProtoAnalysis::printValue(ostream &os, IDEProtoAnalysis::v_t v) const {
  os << llvmIRToString(v);
}

} // namespace psr
