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
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

IFDSSolverTest::IFDSSolverTest(IFDSSolverTest::i_t icfg,
                               vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSSolverTest::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getNormalFlowFunction(IFDSSolverTest::n_t curr,
                                      IFDSSolverTest::n_t succ) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSSolverTest::getNormalFlowFunction()");
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallFlowFunction(IFDSSolverTest::n_t callStmt,
                                    IFDSSolverTest::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSSolverTest::getCallFlowFunction()");
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getRetFlowFunction(IFDSSolverTest::n_t callSite,
                                   IFDSSolverTest::m_t calleeMthd,
                                   IFDSSolverTest::n_t exitStmt,
                                   IFDSSolverTest::n_t retSite) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSSolverTest::getRetFlowFunction()");
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallToRetFlowFunction(IFDSSolverTest::n_t callSite,
                                         IFDSSolverTest::n_t retSite,
                                         set<IFDSSolverTest::m_t> callees) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSSolverTest::getCallToRetFlowFunction()");
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getSummaryFlowFunction(IFDSSolverTest::n_t callStmt,
                                       IFDSSolverTest::m_t destMthd) {
  return nullptr;
}

map<IFDSSolverTest::n_t, set<IFDSSolverTest::d_t>>
IFDSSolverTest::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::initialSeeds()");
  map<IFDSSolverTest::n_t, set<IFDSSolverTest::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IFDSSolverTest::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSSolverTest::d_t IFDSSolverTest::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSSolverTest::isZeroValue(IFDSSolverTest::d_t d) const {
  return isLLVMZeroValue(d);
}

void IFDSSolverTest::printNode(ostream &os, IFDSSolverTest::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSSolverTest::printDataFlowFact(ostream &os,
                                       IFDSSolverTest::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSSolverTest::printMethod(ostream &os, IFDSSolverTest::m_t m) const {
  os << m->getName().str();
}

} // namespace psr
