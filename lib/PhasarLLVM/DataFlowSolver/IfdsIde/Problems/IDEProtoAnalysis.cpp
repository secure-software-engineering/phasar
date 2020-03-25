/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEProtoAnalysis.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

using namespace psr;
using namespace std;

namespace psr {

IDEProtoAnalysis::IDEProtoAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getNormalFlowFunction(IDEProtoAnalysis::n_t curr,
                                        IDEProtoAnalysis::n_t succ) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getCallFlowFunction(IDEProtoAnalysis::n_t callStmt,
                                      IDEProtoAnalysis::f_t destFun) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getRetFlowFunction(IDEProtoAnalysis::n_t callSite,
                                     IDEProtoAnalysis::f_t calleeFun,
                                     IDEProtoAnalysis::n_t exitStmt,
                                     IDEProtoAnalysis::n_t retSite) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getCallToRetFlowFunction(IDEProtoAnalysis::n_t callSite,
                                           IDEProtoAnalysis::n_t retSite,
                                           set<IDEProtoAnalysis::f_t> callees) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::getSummaryFlowFunction(IDEProtoAnalysis::n_t callStmt,
                                         IDEProtoAnalysis::f_t destFun) {
  return nullptr;
}

map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::initialSeeds() {
  cout << "IDEProtoAnalysis::initialSeeds()\n";
  map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IDEProtoAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDEProtoAnalysis::d_t IDEProtoAnalysis::createZeroValue() const {
  cout << "IDEProtoAnalysis::createZeroValue()\n";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDEProtoAnalysis::isZeroValue(IDEProtoAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getNormalEdgeFunction(IDEProtoAnalysis::n_t curr,
                                        IDEProtoAnalysis::d_t currNode,
                                        IDEProtoAnalysis::n_t succ,
                                        IDEProtoAnalysis::d_t succNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallEdgeFunction(IDEProtoAnalysis::n_t callStmt,
                                      IDEProtoAnalysis::d_t srcNode,
                                      IDEProtoAnalysis::f_t destinationFunction,
                                      IDEProtoAnalysis::d_t destNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getReturnEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                        IDEProtoAnalysis::f_t calleeFunction,
                                        IDEProtoAnalysis::n_t exitStmt,
                                        IDEProtoAnalysis::d_t exitNode,
                                        IDEProtoAnalysis::n_t reSite,
                                        IDEProtoAnalysis::d_t retNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallToRetEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                           IDEProtoAnalysis::d_t callNode,
                                           IDEProtoAnalysis::n_t retSite,
                                           IDEProtoAnalysis::d_t retSiteNode,
                                           set<IDEProtoAnalysis::f_t> callees) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getSummaryEdgeFunction(IDEProtoAnalysis::n_t callSite,
                                         IDEProtoAnalysis::d_t callNode,
                                         IDEProtoAnalysis::n_t retSite,
                                         IDEProtoAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::topElement() {
  cout << "IDEProtoAnalysis::topElement()\n";
  return nullptr;
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::bottomElement() {
  cout << "IDEProtoAnalysis::bottomElement()\n";
  return nullptr;
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::join(IDEProtoAnalysis::l_t lhs,
                                             IDEProtoAnalysis::l_t rhs) {
  cout << "IDEProtoAnalysis::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::allTopFunction() {
  cout << "IDEProtoAnalysis::allTopFunction()\n";
  return make_shared<IDEProtoAnalysisAllTop>();
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget(
    IDEProtoAnalysis::l_t source) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> secondFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> otherFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

bool IDEProtoAnalysis::IDEProtoAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> other) const {
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

void IDEProtoAnalysis::printFunction(ostream &os,
                                     IDEProtoAnalysis::f_t m) const {
  os << m->getName().str();
}

void IDEProtoAnalysis::printEdgeFact(ostream &os,
                                     IDEProtoAnalysis::l_t l) const {
  os << llvmIRToString(l);
}

} // namespace psr
