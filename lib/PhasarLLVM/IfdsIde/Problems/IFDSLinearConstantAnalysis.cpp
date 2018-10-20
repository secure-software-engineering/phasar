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
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

size_t hash<LCAPair>::operator()(const LCAPair &k) const {
  return hash<const llvm::Value *>()(k.first) ^ hash<int>()(k.second);
}

namespace psr {

LCAPair::LCAPair() : first(nullptr), second(0) {}

LCAPair::LCAPair(const llvm::Value *V, int i) : first(V), second(i) {}

bool operator==(const LCAPair &lhs, const LCAPair &rhs) {
  return tie(lhs.first, lhs.second) == tie(rhs.first, rhs.second);
}

bool operator!=(const LCAPair &lhs, const LCAPair &rhs) {
  return !(lhs == rhs);
}

bool operator<(const LCAPair &lhs, const LCAPair &rhs) {
  return tie(lhs.first, lhs.second) < tie(rhs.first, rhs.second);
}

IFDSLinearConstantAnalysis::IFDSLinearConstantAnalysis(
    LLVMBasedICFG &icfg, vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSLinearConstantAnalysis::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::getNormalFlowFunction(
    IFDSLinearConstantAnalysis::n_t curr,
    IFDSLinearConstantAnalysis::n_t succ) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::getNormalFlowFunction()");
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::getCallFlowFunction(
    IFDSLinearConstantAnalysis::n_t callStmt,
    IFDSLinearConstantAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::getCallFlowFunction()");
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::getRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t callSite,
    IFDSLinearConstantAnalysis::m_t calleeMthd,
    IFDSLinearConstantAnalysis::n_t exitStmt,
    IFDSLinearConstantAnalysis::n_t retSite) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::getRetFlowFunction()");
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::getCallToRetFlowFunction(
    IFDSLinearConstantAnalysis::n_t callSite,
    IFDSLinearConstantAnalysis::n_t retSite,
    set<IFDSLinearConstantAnalysis::m_t> callees) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::getCallToRetFlowFunction()");
  return Identity<IFDSLinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::getSummaryFlowFunction(
    IFDSLinearConstantAnalysis::n_t callStmt,
    IFDSLinearConstantAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::getSummaryFlowFunction()");
  return nullptr;
}

map<IFDSLinearConstantAnalysis::n_t, set<IFDSLinearConstantAnalysis::d_t>>
IFDSLinearConstantAnalysis::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::initialSeeds()");
  map<IFDSLinearConstantAnalysis::n_t, set<IFDSLinearConstantAnalysis::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                  set<IFDSLinearConstantAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSLinearConstantAnalysis::d_t IFDSLinearConstantAnalysis::createZeroValue() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSLinearConstantAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LCAPair(LLVMZeroValue::getInstance(), 0);
}

bool IFDSLinearConstantAnalysis::isZeroValue(
    IFDSLinearConstantAnalysis::d_t d) const {
  return d == zerovalue;
}

void IFDSLinearConstantAnalysis::printNode(
    ostream &os, IFDSLinearConstantAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSLinearConstantAnalysis::printDataFlowFact(
    ostream &os, IFDSLinearConstantAnalysis::d_t d) const {
  os << '<' + llvmIRToString(d.first) + ", " + to_string(d.second) + '>';
}

void IFDSLinearConstantAnalysis::printMethod(
    ostream &os, IFDSLinearConstantAnalysis::m_t m) const {
  os << m->getName().str();
}

} // namespace psr
