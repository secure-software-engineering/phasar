/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSLinearConstantAnalysis.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSLinearConstantAnalysis::WPDSLinearConstantAnalysis(
    LLVMBasedICFG &I, WPDSType WPDS, SearchDirection Direction,
    std::vector<WPDSLinearConstantAnalysis::n_t> Stack, bool Witnesses)
    : WPDSProblem(I, WPDS, Direction, Stack, Witnesses) {}

shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
WPDSLinearConstantAnalysis::getNormalFlowFunction(
    WPDSLinearConstantAnalysis::n_t curr,
    WPDSLinearConstantAnalysis::n_t succ) {
  return nullptr;
}

shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
WPDSLinearConstantAnalysis::getCallFlowFunction(
    WPDSLinearConstantAnalysis::n_t callStmt,
    WPDSLinearConstantAnalysis::m_t destMthd) {
  return nullptr;
}

shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
WPDSLinearConstantAnalysis::getRetFlowFunction(
    WPDSLinearConstantAnalysis::n_t callSite,
    WPDSLinearConstantAnalysis::m_t calleeMthd,
    WPDSLinearConstantAnalysis::n_t exitStmt,
    WPDSLinearConstantAnalysis::n_t retSite) {
  return nullptr;
}

shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
WPDSLinearConstantAnalysis::getCallToRetFlowFunction(
    WPDSLinearConstantAnalysis::n_t callSite,
    WPDSLinearConstantAnalysis::n_t retSite,
    set<WPDSLinearConstantAnalysis::m_t> callees) {
  return nullptr;
}

shared_ptr<FlowFunction<WPDSLinearConstantAnalysis::d_t>>
WPDSLinearConstantAnalysis::getSummaryFlowFunction(
    WPDSLinearConstantAnalysis::n_t curr,
    WPDSLinearConstantAnalysis::m_t destMthd) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
WPDSLinearConstantAnalysis::getNormalEdgeFunction(
    WPDSLinearConstantAnalysis::n_t curr,
    WPDSLinearConstantAnalysis::d_t currNode,
    WPDSLinearConstantAnalysis::n_t succ,
    WPDSLinearConstantAnalysis::d_t succNode) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
WPDSLinearConstantAnalysis::getCallEdgeFunction(
    WPDSLinearConstantAnalysis::n_t callStmt,
    WPDSLinearConstantAnalysis::d_t srcNode,
    WPDSLinearConstantAnalysis::m_t destiantionMethod,
    WPDSLinearConstantAnalysis::d_t destNode) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
WPDSLinearConstantAnalysis::getReturnEdgeFunction(
    WPDSLinearConstantAnalysis::n_t callSite,
    WPDSLinearConstantAnalysis::m_t calleeMethod,
    WPDSLinearConstantAnalysis::n_t exitStmt,
    WPDSLinearConstantAnalysis::d_t exitNode,
    WPDSLinearConstantAnalysis::n_t reSite,
    WPDSLinearConstantAnalysis::d_t retNode) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
WPDSLinearConstantAnalysis::getCallToRetEdgeFunction(
    WPDSLinearConstantAnalysis::n_t callSite,
    WPDSLinearConstantAnalysis::d_t callNode,
    WPDSLinearConstantAnalysis::n_t retSite,
    WPDSLinearConstantAnalysis::d_t retSiteNode,
    set<WPDSLinearConstantAnalysis::m_t> callees) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSLinearConstantAnalysis::v_t>>
WPDSLinearConstantAnalysis::getSummaryEdgeFunction(
    WPDSLinearConstantAnalysis::n_t curr,
    WPDSLinearConstantAnalysis::d_t currNode,
    WPDSLinearConstantAnalysis::n_t succ,
    WPDSLinearConstantAnalysis::d_t succNode) {
  return nullptr;
}

WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::topElement() {
  return 0;
}
WPDSLinearConstantAnalysis::v_t WPDSLinearConstantAnalysis::bottomElement() {
  return 0;
}
WPDSLinearConstantAnalysis::v_t
WPDSLinearConstantAnalysis::join(WPDSLinearConstantAnalysis::v_t lhs,
                                 WPDSLinearConstantAnalysis::v_t rhs) {
  return 0;
}

WPDSLinearConstantAnalysis::d_t WPDSLinearConstantAnalysis::zeroValue() {
  return LLVMZeroValue::getInstance();
}

} // namespace psr
