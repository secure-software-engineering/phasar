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
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;
namespace psr {

IDESolverTest::IDESolverTest(IDESolverTest::i_t icfg,
                             vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getNormalFlowFunction(IDESolverTest::n_t curr,
                                     IDESolverTest::n_t succ) {
  cout << "IDESolverTest::getNormalFlowFunction()\n";
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getCallFlowFunction(IDESolverTest::n_t callStmt,
                                   IDESolverTest::m_t destMthd) {
  cout << "IDESolverTest::getCallFlowFunction()\n";
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>> IDESolverTest::getRetFlowFunction(
    IDESolverTest::n_t callSite, IDESolverTest::m_t calleeMthd,
    IDESolverTest::n_t exitStmt, IDESolverTest::n_t retSite) {
  cout << "IDESolverTest::getRetFlowFunction()\n";
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getCallToRetFlowFunction(IDESolverTest::n_t callSite,
                                        IDESolverTest::n_t retSite,
                                        set<IDESolverTest::m_t> callees) {
  cout << "IDESolverTest::getCallToRetFlowFunction()\n";
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getSummaryFlowFunction(IDESolverTest::n_t callStmt,
                                      IDESolverTest::m_t destMthd) {
  return nullptr;
}

map<IDESolverTest::n_t, set<IDESolverTest::d_t>> IDESolverTest::initialSeeds() {
  cout << "IDESolverTest::initialSeeds()\n";
  map<IDESolverTest::n_t, set<IDESolverTest::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IDESolverTest::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDESolverTest::d_t IDESolverTest::createZeroValue() {
  cout << "IDESolverTest::createZeroValue()\n";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDESolverTest::isZeroValue(IDESolverTest::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::getNormalEdgeFunction(IDESolverTest::n_t curr,
                                     IDESolverTest::d_t currNode,
                                     IDESolverTest::n_t succ,
                                     IDESolverTest::d_t succNode) {
  cout << "IDESolverTest::getNormalEdgeFunction()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>> IDESolverTest::getCallEdgeFunction(
    IDESolverTest::n_t callStmt, IDESolverTest::d_t srcNode,
    IDESolverTest::m_t destiantionMethod, IDESolverTest::d_t destNode) {
  cout << "IDESolverTest::getCallEdgeFunction()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::getReturnEdgeFunction(IDESolverTest::n_t callSite,
                                     IDESolverTest::m_t calleeMethod,
                                     IDESolverTest::n_t exitStmt,
                                     IDESolverTest::d_t exitNode,
                                     IDESolverTest::n_t reSite,
                                     IDESolverTest::d_t retNode) {
  cout << "IDESolverTest::getReturnEdgeFunction()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::getCallToRetEdgeFunction(IDESolverTest::n_t callSite,
                                        IDESolverTest::d_t callNode,
                                        IDESolverTest::n_t retSite,
                                        IDESolverTest::d_t retSiteNode,
                                        set<IDESolverTest::m_t> callees) {
  cout << "IDESolverTest::getCallToRetEdgeFunction()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::getSummaryEdgeFunction(IDESolverTest::n_t callStmt,
                                      IDESolverTest::d_t callNode,
                                      IDESolverTest::n_t retSite,
                                      IDESolverTest::d_t retSiteNode) {
  cout << "IDESolverTest::getSummaryEdgeFunction()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

IDESolverTest::v_t IDESolverTest::topElement() {
  cout << "IDESolverTest::topElement()\n";
  return nullptr;
}

IDESolverTest::v_t IDESolverTest::bottomElement() {
  cout << "IDESolverTest::bottomElement()\n";
  return nullptr;
}

IDESolverTest::v_t IDESolverTest::join(IDESolverTest::v_t lhs,
                                       IDESolverTest::v_t rhs) {
  cout << "IDESolverTest::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>> IDESolverTest::allTopFunction() {
  cout << "IDESolverTest::allTopFunction()\n";
  return make_shared<IDESolverTestAllTop>();
}

IDESolverTest::v_t
IDESolverTest::IDESolverTestAllTop::computeTarget(IDESolverTest::v_t source) {
  cout << "IDESolverTest::IDESolverTestAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    shared_ptr<EdgeFunction<IDESolverTest::v_t>> secondFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::composeWith()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::v_t>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    shared_ptr<EdgeFunction<IDESolverTest::v_t>> otherFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::joinWith()\n";
  return EdgeIdentity<IDESolverTest::v_t>::getInstance();
}

bool IDESolverTest::IDESolverTestAllTop::equal_to(
    shared_ptr<EdgeFunction<IDESolverTest::v_t>> other) const {
  cout << "IDESolverTest::IDESolverTestAllTop::equalTo()\n";
  return false;
}

void IDESolverTest::printNode(ostream &os, IDESolverTest::n_t n) const {
  os << llvmIRToString(n);
}

void IDESolverTest::printDataFlowFact(ostream &os, IDESolverTest::d_t d) const {
  os << llvmIRToString(d);
}

void IDESolverTest::printMethod(ostream &os, IDESolverTest::m_t m) const {
  os << m->getName().str();
}

void IDESolverTest::printValue(ostream &os, IDESolverTest::v_t v) const {
  os << "empty V test";
}

} // namespace psr
