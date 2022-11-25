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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

namespace psr {

IDESolverTest::IDESolverTest(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IDETabulationProblem::ZeroValue = IDESolverTest::createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getNormalFlowFunction(IDESolverTest::n_t /*Curr*/,
                                     IDESolverTest::n_t /*Succ*/) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getCallFlowFunction(IDESolverTest::n_t /*CallSite*/,
                                   IDESolverTest::f_t /*DestFun*/) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType IDESolverTest::getRetFlowFunction(
    IDESolverTest::n_t /*CallSite*/, IDESolverTest::f_t /*CalleeFun*/,
    IDESolverTest::n_t /*ExitStmt*/, IDESolverTest::n_t /*RetSite*/) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getCallToRetFlowFunction(IDESolverTest::n_t /*CallSite*/,
                                        IDESolverTest::n_t /*RetSite*/,
                                        llvm::ArrayRef<f_t> /*Callees*/) {
  return Identity<IDESolverTest::d_t>::getInstance();
}

IDESolverTest::FlowFunctionPtrType
IDESolverTest::getSummaryFlowFunction(IDESolverTest::n_t /*CallSite*/,
                                      IDESolverTest::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDESolverTest::n_t, IDESolverTest::d_t, IDESolverTest::l_t>
IDESolverTest::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::initialSeeds()");
  InitialSeeds<IDESolverTest::n_t, IDESolverTest::d_t, IDESolverTest::l_t>
      Seeds;
  for (auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), topElement());
  }
  return Seeds;
}

IDESolverTest::d_t IDESolverTest::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDESolverTest::isZeroValue(IDESolverTest::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

// in addition provide specifications for the IDE parts

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getNormalEdgeFunction(IDESolverTest::n_t /*Curr*/,
                                     IDESolverTest::d_t /*CurrNode*/,
                                     IDESolverTest::n_t /*Succ*/,
                                     IDESolverTest::d_t /*SuccNode*/) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getCallEdgeFunction(IDESolverTest::n_t /*CallSite*/,
                                   IDESolverTest::d_t /*SrcNode*/,
                                   IDESolverTest::f_t /*DestinationFunction*/,
                                   IDESolverTest::d_t /*DestNode*/) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getReturnEdgeFunction(IDESolverTest::n_t /*CallSite*/,
                                     IDESolverTest::f_t /*CalleeFunction*/,
                                     IDESolverTest::n_t /*ExitStmt*/,
                                     IDESolverTest::d_t /*ExitNode*/,
                                     IDESolverTest::n_t /*RetSite*/,
                                     IDESolverTest::d_t /*RetNode*/) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getCallToRetEdgeFunction(IDESolverTest::n_t /*CallSite*/,
                                        IDESolverTest::d_t /*CallNode*/,
                                        IDESolverTest::n_t /*RetSite*/,
                                        IDESolverTest::d_t /*RetSiteNode*/,
                                        llvm::ArrayRef<f_t> /*Callees*/) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::getSummaryEdgeFunction(IDESolverTest::n_t /*CallSite*/,
                                      IDESolverTest::d_t /*CallNode*/,
                                      IDESolverTest::n_t /*RetSite*/,
                                      IDESolverTest::d_t /*RetSiteNode*/) {
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

IDESolverTest::l_t IDESolverTest::topElement() {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::topElement()");
  return nullptr;
}

IDESolverTest::l_t IDESolverTest::bottomElement() {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::bottomElement()");
  return nullptr;
}

IDESolverTest::l_t IDESolverTest::join(IDESolverTest::l_t /*Lhs*/,
                                       IDESolverTest::l_t /*Rhs*/) {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::join()");
  return nullptr;
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::allTopFunction() {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::allTopFunction()");
  return std::make_shared<IDESolverTestAllTop>();
}

IDESolverTest::l_t IDESolverTest::IDESolverTestAllTop::computeTarget(
    IDESolverTest::l_t /*Source*/) {
  PHASAR_LOG_LEVEL(DEBUG,
                   "IDESolverTest::IDESolverTestAllTop::computeTarget()");
  return nullptr;
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::composeWith(
    std::shared_ptr<EdgeFunction<IDESolverTest::l_t>> /*SecondFunction*/) {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::IDESolverTestAllTop::composeWith()");
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESolverTest::l_t>>
IDESolverTest::IDESolverTestAllTop::joinWith(
    std::shared_ptr<EdgeFunction<IDESolverTest::l_t>> /*OtherFunction*/) {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::IDESolverTestAllTop::joinWith()");
  return EdgeIdentity<IDESolverTest::l_t>::getInstance();
}

bool IDESolverTest::IDESolverTestAllTop::equal_to(
    std::shared_ptr<EdgeFunction<IDESolverTest::l_t>> /*Other*/) const {
  PHASAR_LOG_LEVEL(DEBUG, "IDESolverTest::IDESolverTestAllTop::equalTo()");
  return false;
}

void IDESolverTest::printNode(llvm::raw_ostream &OS,
                              IDESolverTest::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IDESolverTest::printDataFlowFact(llvm::raw_ostream &OS,
                                      IDESolverTest::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IDESolverTest::printFunction(llvm::raw_ostream &OS,
                                  IDESolverTest::f_t Func) const {
  OS << Func->getName();
}

void IDESolverTest::printEdgeFact(llvm::raw_ostream &OS,
                                  IDESolverTest::l_t /*L*/) const {
  OS << "empty V test";
}

} // namespace psr
