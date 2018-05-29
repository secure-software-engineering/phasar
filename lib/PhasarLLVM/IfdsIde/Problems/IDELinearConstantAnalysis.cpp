/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <limits>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <utility>
using namespace std;

const int IDELinearConstantAnalysis::TOP = std::numeric_limits<int>::min();

const int IDELinearConstantAnalysis::BOTTOM = std::numeric_limits<int>::max();

IDELinearConstantAnalysis::IDELinearConstantAnalysis(LLVMBasedICFG &icfg,
                                                     vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getNormalFlowFunction(
    IDELinearConstantAnalysis::n_t curr, IDELinearConstantAnalysis::n_t succ) {
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::m_t destMthd) {
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::m_t calleeMthd,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::n_t retSite) {
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getCallToRetFlowFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::n_t retSite, std::set<m_t> callees) {
  return Identity<IDELinearConstantAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::getSummaryFlowFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::m_t destMthd) {
  return nullptr;
}

map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
IDELinearConstantAnalysis::initialSeeds() {
  // just start in main()
  map<IDELinearConstantAnalysis::n_t, set<IDELinearConstantAnalysis::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                       set<IDELinearConstantAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDELinearConstantAnalysis::d_t IDELinearConstantAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDELinearConstantAnalysis::isZeroValue(
    IDELinearConstantAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getNormalEdgeFunction(
    IDELinearConstantAnalysis::n_t curr,
    IDELinearConstantAnalysis::d_t currNode,
    IDELinearConstantAnalysis::n_t succ,
    IDELinearConstantAnalysis::d_t succNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getCallEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t srcNode,
    IDELinearConstantAnalysis::m_t destiantionMethod,
    IDELinearConstantAnalysis::d_t destNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getReturnEdgeFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::m_t calleeMethod,
    IDELinearConstantAnalysis::n_t exitStmt,
    IDELinearConstantAnalysis::d_t exitNode,
    IDELinearConstantAnalysis::n_t reSite,
    IDELinearConstantAnalysis::d_t retNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getCallToReturnEdgeFunction(
    IDELinearConstantAnalysis::n_t callSite,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::getSummaryEdgeFunction(
    IDELinearConstantAnalysis::n_t callStmt,
    IDELinearConstantAnalysis::d_t callNode,
    IDELinearConstantAnalysis::n_t retSite,
    IDELinearConstantAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDELinearConstantAnalysis::v_t>::getInstance();
}

IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::topElement() {
  return TOP;
}

IDELinearConstantAnalysis::v_t IDELinearConstantAnalysis::bottomElement() {
  return BOTTOM;
}

IDELinearConstantAnalysis::v_t
IDELinearConstantAnalysis::join(IDELinearConstantAnalysis::v_t lhs,
                                IDELinearConstantAnalysis::v_t rhs) {
  return 0;
}

shared_ptr<EdgeFunction<IDELinearConstantAnalysis::v_t>>
IDELinearConstantAnalysis::allTopFunction() {
  return make_shared<AllTop<IDELinearConstantAnalysis::v_t>>(TOP);
}

string
IDELinearConstantAnalysis::DtoString(IDELinearConstantAnalysis::d_t d) const {
  return llvmIRToString(d);
}

string
IDELinearConstantAnalysis::VtoString(IDELinearConstantAnalysis::v_t v) const {
  return to_string(v);
}

string
IDELinearConstantAnalysis::NtoString(IDELinearConstantAnalysis::n_t n) const {
  return llvmIRToString(n);
}

string
IDELinearConstantAnalysis::MtoString(IDELinearConstantAnalysis::m_t m) const {
  return m->getName().str();
}