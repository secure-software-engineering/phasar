/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEProtoAnalysis.h"
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
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getNormalFlowFunction(IDEProtoAnalysis::n_t Curr,
                                        IDEProtoAnalysis::n_t Succ) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getCallFlowFunction(IDEProtoAnalysis::n_t CallStmt,
                                      IDEProtoAnalysis::f_t DestFun) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType IDEProtoAnalysis::getRetFlowFunction(
    IDEProtoAnalysis::n_t CallSite, IDEProtoAnalysis::f_t CalleeFun,
    IDEProtoAnalysis::n_t ExitStmt, IDEProtoAnalysis::n_t RetSite) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getCallToRetFlowFunction(IDEProtoAnalysis::n_t CallSite,
                                           IDEProtoAnalysis::n_t RetSite,
                                           set<IDEProtoAnalysis::f_t> Callees) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getSummaryFlowFunction(IDEProtoAnalysis::n_t CallStmt,
                                         IDEProtoAnalysis::f_t DestFun) {
  return nullptr;
}

map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>>
IDEProtoAnalysis::initialSeeds() {
  cout << "IDEProtoAnalysis::initialSeeds()\n";
  map<IDEProtoAnalysis::n_t, set<IDEProtoAnalysis::d_t>> SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
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

bool IDEProtoAnalysis::isZeroValue(IDEProtoAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getNormalEdgeFunction(IDEProtoAnalysis::n_t Curr,
                                        IDEProtoAnalysis::d_t CurrNode,
                                        IDEProtoAnalysis::n_t Succ,
                                        IDEProtoAnalysis::d_t SuccNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallEdgeFunction(IDEProtoAnalysis::n_t CallStmt,
                                      IDEProtoAnalysis::d_t SrcNode,
                                      IDEProtoAnalysis::f_t DestinationFunction,
                                      IDEProtoAnalysis::d_t DestNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getReturnEdgeFunction(IDEProtoAnalysis::n_t CallSite,
                                        IDEProtoAnalysis::f_t CalleeFunction,
                                        IDEProtoAnalysis::n_t ExitStmt,
                                        IDEProtoAnalysis::d_t ExitNode,
                                        IDEProtoAnalysis::n_t ReSite,
                                        IDEProtoAnalysis::d_t RetNode) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallToRetEdgeFunction(IDEProtoAnalysis::n_t CallSite,
                                           IDEProtoAnalysis::d_t CallNode,
                                           IDEProtoAnalysis::n_t RetSite,
                                           IDEProtoAnalysis::d_t RetSiteNode,
                                           set<IDEProtoAnalysis::f_t> Callees) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getSummaryEdgeFunction(IDEProtoAnalysis::n_t CallSite,
                                         IDEProtoAnalysis::d_t CallNode,
                                         IDEProtoAnalysis::n_t RetSite,
                                         IDEProtoAnalysis::d_t RetSiteNode) {
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

IDEProtoAnalysis::l_t IDEProtoAnalysis::join(IDEProtoAnalysis::l_t Lhs,
                                             IDEProtoAnalysis::l_t Rhs) {
  cout << "IDEProtoAnalysis::join()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::allTopFunction() {
  cout << "IDEProtoAnalysis::allTopFunction()\n";
  return make_shared<IDEProtoAnalysisAllTop>();
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget(
    IDEProtoAnalysis::l_t Source) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget()\n";
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> SecondFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> OtherFunction) {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith()\n";
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

bool IDEProtoAnalysis::IDEProtoAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> Other) const {
  cout << "IDEProtoAnalysis::IDEProtoAnalysisAllTop::equalTo()\n";
  return false;
}

void IDEProtoAnalysis::printNode(ostream &OS, IDEProtoAnalysis::n_t N) const {
  OS << llvmIRToString(N);
}

void IDEProtoAnalysis::printDataFlowFact(ostream &OS,
                                         IDEProtoAnalysis::d_t D) const {
  OS << llvmIRToString(D);
}

void IDEProtoAnalysis::printFunction(ostream &OS,
                                     IDEProtoAnalysis::f_t M) const {
  OS << M->getName().str();
}

void IDEProtoAnalysis::printEdgeFact(ostream &OS,
                                     IDEProtoAnalysis::l_t L) const {
  OS << llvmIRToString(L);
}

} // namespace psr
