/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSAliasCollector.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSAliasCollector::WPDSAliasCollector(LLVMBasedICFG &I,
                                       const LLVMTypeHierarchy &TH,
                                       const ProjectIRDB &DB, WPDSType WPDS,
                                       SearchDirection Direction,
                                       std::vector<n_t> Stack, bool Witnesses)
    : LLVMDefaultWPDSProblem(I, TH, DB, WPDS, Direction, Stack, Witnesses) {}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getNormalFlowFunction(WPDSAliasCollector::n_t curr,
                                          WPDSAliasCollector::n_t succ) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getCallFlowFunction(WPDSAliasCollector::n_t callStmt,
                                        WPDSAliasCollector::m_t destMthd) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getRetFlowFunction(WPDSAliasCollector::n_t callSite,
                                       WPDSAliasCollector::m_t calleeMthd,
                                       WPDSAliasCollector::n_t exitStmt,
                                       WPDSAliasCollector::n_t retSite) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getCallToRetFlowFunction(
    WPDSAliasCollector::n_t callSite, WPDSAliasCollector::n_t retSite,
    set<WPDSAliasCollector::m_t> callees) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getSummaryFlowFunction(WPDSAliasCollector::n_t curr,
                                           WPDSAliasCollector::m_t destMthd) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::getNormalEdgeFunction(WPDSAliasCollector::n_t curr,
                                          WPDSAliasCollector::d_t currNode,
                                          WPDSAliasCollector::n_t succ,
                                          WPDSAliasCollector::d_t succNode) {
  return EdgeIdentity<WPDSAliasCollector::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::getCallEdgeFunction(
    WPDSAliasCollector::n_t callStmt, WPDSAliasCollector::d_t srcNode,
    WPDSAliasCollector::m_t destinationMethod,
    WPDSAliasCollector::d_t destNode) {
  return EdgeIdentity<WPDSAliasCollector::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::getReturnEdgeFunction(WPDSAliasCollector::n_t callSite,
                                          WPDSAliasCollector::m_t calleeMethod,
                                          WPDSAliasCollector::n_t exitStmt,
                                          WPDSAliasCollector::d_t exitNode,
                                          WPDSAliasCollector::n_t reSite,
                                          WPDSAliasCollector::d_t retNode) {
  return EdgeIdentity<WPDSAliasCollector::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::getCallToRetEdgeFunction(
    WPDSAliasCollector::n_t callSite, WPDSAliasCollector::d_t callNode,
    WPDSAliasCollector::n_t retSite, WPDSAliasCollector::d_t retSiteNode,
    set<WPDSAliasCollector::m_t> callees) {
  return EdgeIdentity<WPDSAliasCollector::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::getSummaryEdgeFunction(WPDSAliasCollector::n_t curr,
                                           WPDSAliasCollector::d_t currNode,
                                           WPDSAliasCollector::n_t succ,
                                           WPDSAliasCollector::d_t succNode) {
  return nullptr;
}

WPDSAliasCollector::v_t WPDSAliasCollector::topElement() {
  return BinaryDomain::TOP;
}
WPDSAliasCollector::v_t WPDSAliasCollector::bottomElement() {
  return BinaryDomain::BOTTOM;
}
WPDSAliasCollector::v_t WPDSAliasCollector::join(WPDSAliasCollector::v_t lhs,
                                                 WPDSAliasCollector::v_t rhs) {
  return (lhs == BinaryDomain::BOTTOM || rhs == BinaryDomain::BOTTOM)
             ? BinaryDomain::BOTTOM
             : BinaryDomain::TOP;
}

WPDSAliasCollector::d_t WPDSAliasCollector::zeroValue() {
  return LLVMZeroValue::getInstance();
}

bool WPDSAliasCollector::isZeroValue(WPDSAliasCollector::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}
std::map<WPDSAliasCollector::n_t, std::set<WPDSAliasCollector::d_t>>
WPDSAliasCollector::initialSeeds() {
  return {};
}

std::shared_ptr<EdgeFunction<WPDSAliasCollector::v_t>>
WPDSAliasCollector::allTopFunction() {
  return make_shared<AllTop<WPDSAliasCollector::v_t>>(BinaryDomain::TOP);
}

void WPDSAliasCollector::printNode(std::ostream &os,
                                   WPDSAliasCollector::n_t n) const {
  os << llvmIRToString(n);
}

void WPDSAliasCollector::printDataFlowFact(std::ostream &os,
                                           WPDSAliasCollector::d_t d) const {
  os << llvmIRToString(d);
}

void WPDSAliasCollector::printMethod(std::ostream &os,
                                     WPDSAliasCollector::m_t m) const {
  os << m->getName().str();
}

void WPDSAliasCollector::printValue(std::ostream &os,
                                    WPDSAliasCollector::v_t v) const {
  if (v == BinaryDomain::TOP) {
    os << "TOP";
  } else {
    os << "BOTTOM";
  }
}

} // namespace psr
