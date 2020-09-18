/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/DefaultSeeds.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;
namespace psr {

IDESolverTest::IDESolverTest(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getNormalFlowFunction(IDESolverTest::n_t Curr,
                                     IDESolverTest::n_t Succ) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getCallFlowFunction(IDESolverTest::n_t CallStmt,
                                   IDESolverTest::f_t DestFun) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType IDESolverTest::getRetFlowFunction(
    IDESolverTest::n_t CallSite, IDESolverTest::f_t CalleeFun,
    IDESolverTest::n_t ExitStmt, IDESolverTest::n_t RetSite) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getCallToRetFlowFunction(IDESolverTest::n_t CallSite,
                                        IDESolverTest::n_t RetSite,
                                        set<IDESolverTest::f_t> Callees) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getSummaryFlowFunction(IDESolverTest::n_t CallStmt,
                                      IDESolverTest::f_t DestFun) {
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

bool IDESolverTest::isZeroValue(IDESolverTest::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getNormalEdgeFunction(IDESolverTest::n_t Curr,
                                     IDESolverTest::d_t CurrNode,
                                     IDESolverTest::n_t Succ,
                                     IDESolverTest::d_t SuccNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>> IDESolverTest::getCallEdgeFunction(
    IDESolverTest::n_t CallStmt, IDESolverTest::d_t SrcNode,
    IDESolverTest::f_t DestinationFunction, IDESolverTest::d_t DestNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getReturnEdgeFunction(IDESolverTest::n_t CallSite,
                                     IDESolverTest::f_t CalleeFunction,
                                     IDESolverTest::n_t ExitStmt,
                                     IDESolverTest::d_t ExitNode,
                                     IDESolverTest::n_t ReSite,
                                     IDESolverTest::d_t RetNode) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getCallToRetEdgeFunction(IDESolverTest::n_t CallSite,
                                        IDESolverTest::d_t CallNode,
                                        IDESolverTest::n_t RetSite,
                                        IDESolverTest::d_t RetSiteNode,
                                        set<IDESolverTest::f_t> Callees) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getSummaryEdgeFunction(IDESolverTest::n_t CallStmt,
                                      IDESolverTest::d_t CallNode,
                                      IDESolverTest::n_t RetSite,
                                      IDESolverTest::d_t RetSiteNode) {
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

IDESolverTest::l_t IDESolverTest::join(IDESolverTest::l_t Lhs,
                                       IDESolverTest::l_t Rhs) {
  cout << "IDESolverTest::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>> IDESolverTest::allTopFunction() {
  cout << "IDESolverTest::allTopFunction()\n";
  return make_shared<IDESolverTestAllTop>();
}

IDESolverTest::l_t
IDESolverTest::IDESolverTestAllTop::computeTarget(IDESolverTest::l_t Source) {
  cout << "IDESolverTest::IDESolverTestAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> SecondFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::composeWith()\n";
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> OtherFunction) {
  cout << "IDESolverTest::IDESolverTestAllTop::joinWith()\n";
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

bool IDESolverTest::IDESolverTestAllTop::equal_to(
    shared_ptr<EdgeFunction<IDESolverTest::l_t>> Other) const {
  cout << "IDESolverTest::IDESolverTestAllTop::equalTo()\n";
  return false;
}

void IDESolverTest::printNode(ostream &OS, IDESolverTest::n_t N) const {
  OS << llvmIRToString(N);
}

void IDESolverTest::printDataFlowFact(ostream &OS, IDESolverTest::d_t D) const {
  OS << llvmIRToString(D);
}

void IDESolverTest::printFunction(ostream &OS, IDESolverTest::f_t M) const {
  OS << M->getName().str();
}

void IDESolverTest::printEdgeFact(ostream &OS, IDESolverTest::l_t L) const {
  OS << "empty V test";
}

} // namespace psr
