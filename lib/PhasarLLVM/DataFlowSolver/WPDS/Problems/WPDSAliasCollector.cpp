/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSAliasCollector.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

WPDSAliasCollector::WPDSAliasCollector(const ProjectIRDB *IRDB,
                                       const LLVMTypeHierarchy *TH,
                                       const LLVMBasedICFG *ICF,
                                       const LLVMPointsToInfo *PT,
                                       std::set<std::string> EntryPoints)
    : WPDSProblem<WPDSAliasCollector::n_t, WPDSAliasCollector::d_t,
                  WPDSAliasCollector::f_t, WPDSAliasCollector::t_t,
                  WPDSAliasCollector::v_t, WPDSAliasCollector::l_t,
                  WPDSAliasCollector::i_t>(IRDB, TH, ICF, PT, EntryPoints) {}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getNormalFlowFunction(WPDSAliasCollector::n_t curr,
                                          WPDSAliasCollector::n_t succ) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getCallFlowFunction(WPDSAliasCollector::n_t callStmt,
                                        WPDSAliasCollector::f_t destFun) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getRetFlowFunction(WPDSAliasCollector::n_t callSite,
                                       WPDSAliasCollector::f_t calleeFun,
                                       WPDSAliasCollector::n_t exitStmt,
                                       WPDSAliasCollector::n_t retSite) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getCallToRetFlowFunction(
    WPDSAliasCollector::n_t callSite, WPDSAliasCollector::n_t retSite,
    set<WPDSAliasCollector::f_t> callees) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSAliasCollector::d_t>>
WPDSAliasCollector::getSummaryFlowFunction(WPDSAliasCollector::n_t curr,
                                           WPDSAliasCollector::f_t destFun) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getNormalEdgeFunction(WPDSAliasCollector::n_t curr,
                                          WPDSAliasCollector::d_t currNode,
                                          WPDSAliasCollector::n_t succ,
                                          WPDSAliasCollector::d_t succNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getCallEdgeFunction(
    WPDSAliasCollector::n_t callStmt, WPDSAliasCollector::d_t srcNode,
    WPDSAliasCollector::f_t destinationFunction,
    WPDSAliasCollector::d_t destNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getReturnEdgeFunction(
    WPDSAliasCollector::n_t callSite, WPDSAliasCollector::f_t calleeFunction,
    WPDSAliasCollector::n_t exitStmt, WPDSAliasCollector::d_t exitNode,
    WPDSAliasCollector::n_t reSite, WPDSAliasCollector::d_t retNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getCallToRetEdgeFunction(
    WPDSAliasCollector::n_t callSite, WPDSAliasCollector::d_t callNode,
    WPDSAliasCollector::n_t retSite, WPDSAliasCollector::d_t retSiteNode,
    set<WPDSAliasCollector::f_t> callees) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getSummaryEdgeFunction(WPDSAliasCollector::n_t curr,
                                           WPDSAliasCollector::d_t currNode,
                                           WPDSAliasCollector::n_t succ,
                                           WPDSAliasCollector::d_t succNode) {
  return nullptr;
}

WPDSAliasCollector::l_t WPDSAliasCollector::topElement() {
  return BinaryDomain::TOP;
}
WPDSAliasCollector::l_t WPDSAliasCollector::bottomElement() {
  return BinaryDomain::BOTTOM;
}
WPDSAliasCollector::l_t WPDSAliasCollector::join(WPDSAliasCollector::l_t lhs,
                                                 WPDSAliasCollector::l_t rhs) {
  return (lhs == BinaryDomain::BOTTOM || rhs == BinaryDomain::BOTTOM)
             ? BinaryDomain::BOTTOM
             : BinaryDomain::TOP;
}

bool WPDSAliasCollector::isZeroValue(WPDSAliasCollector::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}
std::map<WPDSAliasCollector::n_t, std::set<WPDSAliasCollector::d_t>>
WPDSAliasCollector::initialSeeds() {
  return {};
}

std::shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::allTopFunction() {
  return make_shared<AllTop<WPDSAliasCollector::l_t>>(BinaryDomain::TOP);
}

void WPDSAliasCollector::printNode(std::ostream &os,
                                   WPDSAliasCollector::n_t n) const {
  os << llvmIRToString(n);
}

void WPDSAliasCollector::printDataFlowFact(std::ostream &os,
                                           WPDSAliasCollector::d_t d) const {
  os << llvmIRToString(d);
}

void WPDSAliasCollector::printFunction(std::ostream &os,
                                       WPDSAliasCollector::f_t m) const {
  os << m->getName().str();
}

void WPDSAliasCollector::printEdgeFact(std::ostream &os,
                                       WPDSAliasCollector::l_t v) const {
  if (v == BinaryDomain::TOP) {
    os << "TOP";
  } else {
    os << "BOTTOM";
  }
}

} // namespace psr
