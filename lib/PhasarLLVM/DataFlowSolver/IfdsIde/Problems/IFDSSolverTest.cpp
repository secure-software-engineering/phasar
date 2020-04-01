/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
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
                               const LLVMBasedICFG *ICF,
                               const LLVMPointsToInfo *PT,
                               std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IFDSSolverTest::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getNormalFlowFunction(IFDSSolverTest::n_t curr,
                                      IFDSSolverTest::n_t succ) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallFlowFunction(IFDSSolverTest::n_t callStmt,
                                    IFDSSolverTest::f_t destFun) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getRetFlowFunction(IFDSSolverTest::n_t callSite,
                                   IFDSSolverTest::f_t calleeFun,
                                   IFDSSolverTest::n_t exitStmt,
                                   IFDSSolverTest::n_t retSite) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallToRetFlowFunction(IFDSSolverTest::n_t callSite,
                                         IFDSSolverTest::n_t retSite,
                                         set<IFDSSolverTest::f_t> callees) {
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getSummaryFlowFunction(IFDSSolverTest::n_t callStmt,
                                       IFDSSolverTest::f_t destFun) {
  return nullptr;
}

map<IFDSSolverTest::n_t, set<IFDSSolverTest::d_t>>
IFDSSolverTest::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::initialSeeds()");
  map<IFDSSolverTest::n_t, set<IFDSSolverTest::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IFDSSolverTest::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IFDSSolverTest::d_t IFDSSolverTest::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSolverTest::isZeroValue(IFDSSolverTest::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSSolverTest::printNode(ostream &os, IFDSSolverTest::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSSolverTest::printDataFlowFact(ostream &os,
                                       IFDSSolverTest::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSSolverTest::printFunction(ostream &os, IFDSSolverTest::f_t m) const {
  os << m->getName().str();
}

} // namespace psr
