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
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
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

IDEProtoAnalysis::IDEProtoAnalysis(const LLVMProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue = IDEProtoAnalysis::createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getNormalFlowFunction(IDEProtoAnalysis::n_t /*Curr*/,
                                        IDEProtoAnalysis::n_t /*Succ*/) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getCallFlowFunction(IDEProtoAnalysis::n_t /*CallSite*/,
                                      IDEProtoAnalysis::f_t /*DestFun*/) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType IDEProtoAnalysis::getRetFlowFunction(
    IDEProtoAnalysis::n_t /*CallSite*/, IDEProtoAnalysis::f_t /*CalleeFun*/,
    IDEProtoAnalysis::n_t /*ExitSite*/, IDEProtoAnalysis::n_t /*RetSite*/) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getCallToRetFlowFunction(
    IDEProtoAnalysis::n_t /*CallSite*/, IDEProtoAnalysis::n_t /*RetSite*/,
    set<IDEProtoAnalysis::f_t> /*Callees*/) {
  return Identity<IDEProtoAnalysis::d_t>::getInstance();
}

IDEProtoAnalysis::FlowFunctionPtrType
IDEProtoAnalysis::getSummaryFlowFunction(IDEProtoAnalysis::n_t /*CallSite*/,
                                         IDEProtoAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDEProtoAnalysis::n_t, IDEProtoAnalysis::d_t,
             IDEProtoAnalysis::l_t>
IDEProtoAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::initialSeeds()");
  InitialSeeds<IDEProtoAnalysis::n_t, IDEProtoAnalysis::d_t,
               IDEProtoAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), bottomElement());
  }
  return Seeds;
}

IDEProtoAnalysis::d_t IDEProtoAnalysis::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDEProtoAnalysis::isZeroValue(IDEProtoAnalysis::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getNormalEdgeFunction(IDEProtoAnalysis::n_t /*Curr*/,
                                        IDEProtoAnalysis::d_t /*CurrNode*/,
                                        IDEProtoAnalysis::n_t /*Succ*/,
                                        IDEProtoAnalysis::d_t /*SuccNode*/) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallEdgeFunction(
    IDEProtoAnalysis::n_t /*CallSite*/, IDEProtoAnalysis::d_t /*SrcNode*/,
    IDEProtoAnalysis::f_t /*DestinationFunction*/,
    IDEProtoAnalysis::d_t /*DestNode*/) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getReturnEdgeFunction(
    IDEProtoAnalysis::n_t /*CallSite*/,
    IDEProtoAnalysis::f_t /*CalleeFunction*/,
    IDEProtoAnalysis::n_t /*ExitSite*/, IDEProtoAnalysis::d_t /*ExitNode*/,
    IDEProtoAnalysis::n_t /*ReSite*/, IDEProtoAnalysis::d_t /*RetNode*/) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getCallToRetEdgeFunction(
    IDEProtoAnalysis::n_t /*CallSite*/, IDEProtoAnalysis::d_t /*CallNode*/,
    IDEProtoAnalysis::n_t /*RetSite*/, IDEProtoAnalysis::d_t /*RetSiteNode*/,
    set<IDEProtoAnalysis::f_t> /*Callees*/) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::getSummaryEdgeFunction(
    IDEProtoAnalysis::n_t /*CallSite*/, IDEProtoAnalysis::d_t /*CallNode*/,
    IDEProtoAnalysis::n_t /*RetSite*/, IDEProtoAnalysis::d_t /*RetSiteNode*/) {
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::topElement() {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::topElement()");
  return nullptr;
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::bottomElement() {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::bottomElement()");
  return nullptr;
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::join(IDEProtoAnalysis::l_t /*Lhs*/,
                                             IDEProtoAnalysis::l_t /*Rhs*/) {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::join()");
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::allTopFunction() {
  PHASAR_LOG_LEVEL(DEBUG, "IDEProtoAnalysis::allTopFunction()");
  return make_shared<IDEProtoAnalysisAllTop>();
}

IDEProtoAnalysis::l_t IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget(
    IDEProtoAnalysis::l_t /*Source*/) {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IDEProtoAnalysis::IDEProtoAnalysisAllTop::computeTarget()");
  return nullptr;
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> /*SecondFunction*/) {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IDEProtoAnalysis::IDEProtoAnalysisAllTop::composeWith()");
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>>
IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> /*OtherFunction*/) {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IDEProtoAnalysis::IDEProtoAnalysisAllTop::joinWith()");
  return EdgeIdentity<IDEProtoAnalysis::l_t>::getInstance();
}

bool IDEProtoAnalysis::IDEProtoAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDEProtoAnalysis::l_t>> /*Other*/) const {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IDEProtoAnalysis::IDEProtoAnalysisAllTop::equalTo()");
  return false;
}

void IDEProtoAnalysis::printNode(llvm::raw_ostream &OS,
                                 IDEProtoAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IDEProtoAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                         IDEProtoAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IDEProtoAnalysis::printFunction(llvm::raw_ostream &OS,
                                     IDEProtoAnalysis::f_t Func) const {
  OS << Func->getName();
}

void IDEProtoAnalysis::printEdgeFact(llvm::raw_ostream &OS,
                                     IDEProtoAnalysis::l_t L) const {
  OS << llvmIRToString(L);
}

} // namespace psr
