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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

bool IDETaintAnalysis::setContainsStr(set<string> S, const string &Str) {
  return S.find(Str) != S.end();
}

IDETaintAnalysis::IDETaintAnalysis(const LLVMProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue = IDETaintAnalysis::createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

IDETaintAnalysis::FlowFunctionPtrType
IDETaintAnalysis::getNormalFlowFunction(IDETaintAnalysis::n_t /*Curr*/,
                                        IDETaintAnalysis::n_t /*Succ*/) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

IDETaintAnalysis::FlowFunctionPtrType
IDETaintAnalysis::getCallFlowFunction(IDETaintAnalysis::n_t /*CallSite*/,
                                      IDETaintAnalysis::f_t /*DestFun*/) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

IDETaintAnalysis::FlowFunctionPtrType IDETaintAnalysis::getRetFlowFunction(
    IDETaintAnalysis::n_t /*CallSite*/, IDETaintAnalysis::f_t /*CalleeFun*/,
    IDETaintAnalysis::n_t /*ExitStmt*/, IDETaintAnalysis::n_t /*RetSite*/) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

IDETaintAnalysis::FlowFunctionPtrType
IDETaintAnalysis::getCallToRetFlowFunction(IDETaintAnalysis::n_t /*CallSite*/,
                                           IDETaintAnalysis::n_t /*RetSite*/,
                                           llvm::ArrayRef<f_t> /*Callees*/) {
  return Identity<IDETaintAnalysis::d_t>::getInstance();
}

IDETaintAnalysis::FlowFunctionPtrType
IDETaintAnalysis::getSummaryFlowFunction(IDETaintAnalysis::n_t /*CallSite*/,
                                         IDETaintAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDETaintAnalysis::n_t, IDETaintAnalysis::d_t,
             IDETaintAnalysis::l_t>
IDETaintAnalysis::initialSeeds() {
  // just start in main()
  InitialSeeds<IDETaintAnalysis::n_t, IDETaintAnalysis::d_t,
               IDETaintAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), bottomElement());
  }
  return Seeds;
}

IDETaintAnalysis::d_t IDETaintAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETaintAnalysis::isZeroValue(IDETaintAnalysis::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getNormalEdgeFunction(IDETaintAnalysis::n_t /*Curr*/,
                                        IDETaintAnalysis::d_t /*CurrNode*/,
                                        IDETaintAnalysis::n_t /*Succ*/,
                                        IDETaintAnalysis::d_t /*SuccNode*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getCallEdgeFunction(
    IDETaintAnalysis::n_t /*CallSite*/, IDETaintAnalysis::d_t /*SrcNode*/,
    IDETaintAnalysis::f_t /*DestinationFunction*/,
    IDETaintAnalysis::d_t /*DestNode*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getReturnEdgeFunction(
    IDETaintAnalysis::n_t /*CallSite*/,
    IDETaintAnalysis::f_t /*CalleeFunction*/,
    IDETaintAnalysis::n_t /*ExitSite*/, IDETaintAnalysis::d_t /*ExitNode*/,
    IDETaintAnalysis::n_t /*ReSite*/, IDETaintAnalysis::d_t /*RetNode*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getCallToRetEdgeFunction(
    IDETaintAnalysis::n_t /*CallSite*/, IDETaintAnalysis::d_t /*CallNode*/,
    IDETaintAnalysis::n_t /*RetSite*/, IDETaintAnalysis::d_t /*RetSiteNode*/,
    llvm::ArrayRef<f_t> /*Callees*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::getSummaryEdgeFunction(
    IDETaintAnalysis::n_t /*CallSite*/, IDETaintAnalysis::d_t /*CallNode*/,
    IDETaintAnalysis::n_t /*RetSite*/, IDETaintAnalysis::d_t /*RetSiteNode*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

IDETaintAnalysis::l_t IDETaintAnalysis::topElement() { return nullptr; }

IDETaintAnalysis::l_t IDETaintAnalysis::bottomElement() { return nullptr; }

IDETaintAnalysis::l_t IDETaintAnalysis::join(IDETaintAnalysis::l_t /*Lhs*/,
                                             IDETaintAnalysis::l_t /*Rhs*/) {
  return nullptr;
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::allTopFunction() {
  return make_shared<IDETainAnalysisAllTop>();
}

IDETaintAnalysis::l_t IDETaintAnalysis::IDETainAnalysisAllTop::computeTarget(
    IDETaintAnalysis::l_t /*Source*/) {
  return nullptr;
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::IDETainAnalysisAllTop::composeWith(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> /*SecondFunction*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>>
IDETaintAnalysis::IDETainAnalysisAllTop::joinWith(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> /*OtherFunction*/) {
  return EdgeIdentity<IDETaintAnalysis::l_t>::getInstance();
}

bool IDETaintAnalysis::IDETainAnalysisAllTop::equal_to(
    shared_ptr<EdgeFunction<IDETaintAnalysis::l_t>> /*Other*/) const {
  return false;
}

void IDETaintAnalysis::printNode(llvm::raw_ostream &OS,
                                 IDETaintAnalysis::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IDETaintAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                         IDETaintAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IDETaintAnalysis::printFunction(llvm::raw_ostream &OS,
                                     IDETaintAnalysis::f_t Func) const {
  OS << Func->getName();
}

void IDETaintAnalysis::printEdgeFact(llvm::raw_ostream &OS,
                                     IDETaintAnalysis::l_t L) const {
  OS << llvmIRToString(L);
}

} // namespace psr
