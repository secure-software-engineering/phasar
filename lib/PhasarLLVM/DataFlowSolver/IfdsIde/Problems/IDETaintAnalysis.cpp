/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

using namespace std;
using namespace psr;
namespace psr {

bool IDETaintAnalysis::set_contains_str(set<string> s, string str) {
  return s.find(str) != s.end();
}

IDETaintAnalysis::IDETaintAnalysis(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETaintAnalysis::d_t>>
IDETaintAnalysis::getNormalFlowFunction(IDETaintAnalysis::n_t curr,
                                        IDETaintAnalysis::n_t succ) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETaintAnalysis::d_t>>
IDETaintAnalysis::getCallFlowFunction(IDETaintAnalysis::n_t callStmt,
                                      IDETaintAnalysis::f_t destFun) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETaintAnalysis::d_t>>
IDETaintAnalysis::getRetFlowFunction(IDETaintAnalysis::n_t callSite,
                                     IDETaintAnalysis::f_t calleeFun,
                                     IDETaintAnalysis::n_t exitStmt,
                                     IDETaintAnalysis::n_t retSite) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETaintAnalysis::d_t>>
IDETaintAnalysis::getCallToRetFlowFunction(IDETaintAnalysis::n_t callSite,
                                           IDETaintAnalysis::n_t retSite,
                                           set<IDETaintAnalysis::f_t> callees) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETaintAnalysis::d_t>>
IDETaintAnalysis::getSummaryFlowFunction(IDETaintAnalysis::n_t callStmt,
                                         IDETaintAnalysis::f_t destFun) {
  return nullptr;
}

map<IDETaintAnalysis::n_t, set<IDETaintAnalysis::d_t>>
IDETaintAnalysis::initialSeeds() {
  // just start in main()
  map<IDETaintAnalysis::n_t, set<IDETaintAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IDETaintAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDETaintAnalysis::d_t IDETaintAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETaintAnalysis::isZeroValue(IDETaintAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getNormalEdgeFunction(IDETaintAnalysis::n_t curr,
                                        IDETaintAnalysis::d_t currNode,
                                        IDETaintAnalysis::n_t succ,
                                        IDETaintAnalysis::d_t succNode) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getCallEdgeFunction(IDETaintAnalysis::n_t callStmt,
                                      IDETaintAnalysis::d_t srcNode,
                                      IDETaintAnalysis::f_t destinationFunction,
                                      IDETaintAnalysis::d_t destNode) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getReturnEdgeFunction(IDETaintAnalysis::n_t callSite,
                                        IDETaintAnalysis::f_t calleeFunction,
                                        IDETaintAnalysis::n_t exitStmt,
                                        IDETaintAnalysis::d_t exitNode,
                                        IDETaintAnalysis::n_t reSite,
                                        IDETaintAnalysis::d_t retNode) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getCallToRetEdgeFunction(IDETaintAnalysis::n_t callSite,
                                           IDETaintAnalysis::d_t callNode,
                                           IDETaintAnalysis::n_t retSite,
                                           IDETaintAnalysis::d_t retSiteNode,
                                           set<IDETaintAnalysis::f_t> callees) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getSummaryEdgeFunction(IDETaintAnalysis::n_t callStmt,
                                         IDETaintAnalysis::d_t callNode,
                                         IDETaintAnalysis::n_t retSite,
                                         IDETaintAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

IDETaintAnalysis::l_t IDETaintAnalysis::topElement() { return nullptr; }

IDETaintAnalysis::l_t IDETaintAnalysis::bottomElement() { return nullptr; }

IDETaintAnalysis::l_t IDETaintAnalysis::join(IDETaintAnalysis::l_t lhs,
                                             IDETaintAnalysis::l_t rhs) {
  return nullptr;
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::allTopFunction() {
  return make_shared<IDETainAnalysisAllTop>();
}

IDETaintAnalysis::l_t IDETaintAnalysis::IDETainAnalysisAllTop::computeTarget(
    IDETaintAnalysis::l_t source) {
  return nullptr;
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::IDETainAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> secondFunction) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::IDETainAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> otherFunction) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

bool IDETaintAnalysis::IDETainAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> other) const {
  return false;
}

void IDETaintAnalysis::printNode(ostream &os, IDETaintAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IDETaintAnalysis::printDataFlowFact(ostream &os,
                                         IDETaintAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IDETaintAnalysis::printFunction(ostream &os,
                                     IDETaintAnalysis::f_t m) const {
  os << m->getName().str();
}

void IDETaintAnalysis::printEdgeFact(ostream &os,
                                     IDETaintAnalysis::l_t v) const {
  os << llvmIRToString(v);
}

} // namespace psr
