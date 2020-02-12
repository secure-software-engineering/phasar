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
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/DefaultSeeds.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;
namespace psr {

IDESolverTest::IDESolverTest(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF,
                             const LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getNormalFlowFunction(IDESolverTest::n_t curr,
                                     IDESolverTest::n_t succ) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getCallFlowFunction(IDESolverTest::n_t callStmt,
                                   IDESolverTest::f_t destFun) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>> IDESolverTest::getRetFlowFunction(
    IDESolverTest::n_t callSite, IDESolverTest::f_t calleeFun,
    IDESolverTest::n_t exitStmt, IDESolverTest::n_t retSite) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getCallToRetFlowFunction(IDESolverTest::n_t callSite,
                                        IDESolverTest::n_t retSite,
                                        set<IDESolverTest::f_t> callees) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDESolverTest::d_t>>
IDESolverTest::getSummaryFlowFunction(IDESolverTest::n_t callStmt,
                                      IDESolverTest::f_t destFun) {
  return nullptr;
}

map<IDESolverTest::n_t, set<IDESolverTest::d_t>> IDESolverTest::initialSeeds() {
  cout << "IDESolverTest::initialSeeds()\n";
  map<IDESolverTest::n_t, set<IDESolverTest::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IDESolverTest::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDESolverTest::d_t IDESolverTest::createZeroValue() const {
  cout << "IDESolverTest::createZeroValue()\n";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDESolverTest::isZeroValue(IDESolverTest::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getNormalEdgeFunction(IDESolverTest::n_t curr,
                                     IDESolverTest::d_t currNode,
                                     IDESolverTest::n_t succ,
                                     IDESolverTest::d_t succNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>> IDESolverTest::getCallEdgeFunction(
    IDESolverTest::n_t callStmt, IDESolverTest::d_t srcNode,
    IDESolverTest::f_t destinationFunction, IDESolverTest::d_t destNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getReturnEdgeFunction(IDESolverTest::n_t callSite,
                                     IDESolverTest::f_t calleeFunction,
                                     IDESolverTest::n_t exitStmt,
                                     IDESolverTest::d_t exitNode,
                                     IDESolverTest::n_t reSite,
                                     IDESolverTest::d_t retNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getCallToRetEdgeFunction(IDESolverTest::n_t callSite,
                                        IDESolverTest::d_t callNode,
                                        IDESolverTest::n_t retSite,
                                        IDESolverTest::d_t retSiteNode,
                                        set<IDESolverTest::f_t> callees) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getSummaryEdgeFunction(IDESolverTest::n_t callStmt,
                                      IDESolverTest::d_t callNode,
                                      IDESolverTest::n_t retSite,
                                      IDESolverTest::d_t retSiteNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

IDESolverTest::l_t IDESolverTest::topElement() {
  cout << "IDESolverTest::topElement()\n";
  return nullptr;
}

IDESolverTest::l_t IDESolverTest::bottomElement() {
  cout << "IDESolverTest::bottomElement()\n";
  return nullptr;
}

IDESolverTest::l_t IDESolverTest::join(IDESolverTest::l_t lhs,
                                       IDESolverTest::l_t rhs) {
  cout << "IDESolverTest::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>> IDESolverTest::allTopFunction() {
  cout << "IDESolverTest::allTopFunction()\n";
  return make_shared<IDESolverTestAllTop>();
}

IDESolverTest::l_t
IDESolverTest::IDESolverTestAllTop::computeTarget(IDESolverTest::l_t source) {
  cout << "IDESolverTest::IDESolverTestAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> secondFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::composeWith()\n";
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> otherFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::joinWith()\n";
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

bool IDESolverTest::IDESolverTestAllTop::equal_to(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> other) const {
  cout << "IDESolverTest::IDESolverTestAllTop::equalTo()\n";
  return false;
}

void IDESolverTest::printNode(ostream &os, IDESolverTest::n_t n) const {
  os << llvmIRToString(n);
}

void IDESolverTest::printDataFlowFact(ostream &os, IDESolverTest::d_t d) const {
  os << llvmIRToString(d);
}

void IDESolverTest::printFunction(ostream &os, IDESolverTest::f_t m) const {
  os << m->getName().str();
}

void IDESolverTest::printEdgeFact(ostream &os, IDESolverTest::l_t l) const {
  os << "empty V test";
}

} // namespace psr
