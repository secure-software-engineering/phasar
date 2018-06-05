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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>
#include <phasar/Utils/LLVMShorthands.h>
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
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getNormalFlowFunction()";
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallFlowFunction(IFDSSolverTest::n_t callStmt,
                                    IFDSSolverTest::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getCallFlowFunction()";
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getRetFlowFunction(IFDSSolverTest::n_t callSite,
                                   IFDSSolverTest::m_t calleeMthd,
                                   IFDSSolverTest::n_t exitStmt,
                                   IFDSSolverTest::n_t retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getRetFlowFunction()";
  return Identity<IFDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSSolverTest::d_t>>
IFDSSolverTest::getCallToRetFlowFunction(IFDSSolverTest::n_t callSite,
                                         IFDSSolverTest::n_t retSite,
                                         set<IFDSSolverTest::m_t> callees) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::getCallToRetFlowFunction()";
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
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSSolverTest::initialSeeds()";
  map<IFDSSolverTest::n_t, set<IFDSSolverTest::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
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

string IFDSSolverTest::DtoString(IFDSSolverTest::d_t d) const {
  return llvmIRToString(d);
}

string IFDSSolverTest::NtoString(IFDSSolverTest::n_t n) const {
  return llvmIRToString(n);
}

string IFDSSolverTest::MtoString(IFDSSolverTest::m_t m) const {
  return m->getName().str();
}
} // namespace psr
