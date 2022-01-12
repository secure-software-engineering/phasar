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
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSSolverTest::IFDSSolverTest(const ProjectIRDB *IRDB,
                               const LLVMTypeHierarchy *TH,
                               const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                               std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)) {
  IFDSSolverTest::ZeroValue = IFDSSolverTest::createZeroValue();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getNormalFlowFunction(IFDSSolverTest::n_t /*Curr*/,
                                      IFDSSolverTest::n_t /*Succ*/) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getCallFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                    IFDSSolverTest::f_t /*DestFun*/) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

IFDSSolverTest::FlowFunctionPtrType IFDSSolverTest::getRetFlowFunction(
    IFDSSolverTest::n_t /*CallSite*/, IFDSSolverTest::f_t /*CalleeFun*/,
    IFDSSolverTest::n_t /*ExitStmt*/, IFDSSolverTest::n_t /*RetSite*/) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getCallToRetFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                         IFDSSolverTest::n_t /*RetSite*/,
                                         set<IFDSSolverTest::f_t> /*Callees*/) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

IFDSSolverTest::FlowFunctionPtrType
IFDSSolverTest::getSummaryFlowFunction(IFDSSolverTest::n_t /*CallSite*/,
                                       IFDSSolverTest::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IFDSSolverTest::n_t, IFDSSolverTest::d_t, IFDSSolverTest::l_t>
IFDSSolverTest::initialSeeds() {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IFDSSolverTest::initialSeeds()");
  InitialSeeds<IFDSSolverTest::n_t, IFDSSolverTest::d_t, IFDSSolverTest::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
  }
  return Seeds;
}

IFDSSolverTest::d_t IFDSSolverTest::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSolverTest::isZeroValue(IFDSSolverTest::d_t Fact) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(Fact);
}

void IFDSSolverTest::printNode(ostream &OS, IFDSSolverTest::n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IFDSSolverTest::printDataFlowFact(ostream &OS,
                                       IFDSSolverTest::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IFDSSolverTest::printFunction(ostream &OS,
                                   IFDSSolverTest::f_t Func) const {
  OS << Func->getName().str();
}

} // namespace psr
