/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSSolverTest.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSSolverTest::WPDSSolverTest(const ProjectIRDB *IRDB,
                               const LLVMTypeHierarchy *TH,
                               const LLVMBasedICFG *ICF,
                               const LLVMPointsToInfo *PT,
                               std::set<std::string> EntryPoints)
    : WPDSProblem<WPDSSolverTest::n_t, WPDSSolverTest::d_t, WPDSSolverTest::f_t,
                  WPDSSolverTest::t_t, WPDSSolverTest::v_t, WPDSSolverTest::l_t,
                  WPDSSolverTest::i_t>(IRDB, TH, ICF, PT, EntryPoints) {}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getNormalFlowFunction(WPDSSolverTest::n_t curr,
                                      WPDSSolverTest::n_t succ) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getCallFlowFunction(WPDSSolverTest::n_t callStmt,
                                    WPDSSolverTest::f_t destFun) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getRetFlowFunction(WPDSSolverTest::n_t callSite,
                                   WPDSSolverTest::f_t calleeFun,
                                   WPDSSolverTest::n_t exitStmt,
                                   WPDSSolverTest::n_t retSite) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getCallToRetFlowFunction(WPDSSolverTest::n_t callSite,
                                         WPDSSolverTest::n_t retSite,
                                         set<WPDSSolverTest::f_t> callees) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getSummaryFlowFunction(WPDSSolverTest::n_t curr,
                                       WPDSSolverTest::f_t destFun) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getNormalEdgeFunction(WPDSSolverTest::n_t curr,
                                      WPDSSolverTest::d_t currNode,
                                      WPDSSolverTest::n_t succ,
                                      WPDSSolverTest::d_t succNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getCallEdgeFunction(WPDSSolverTest::n_t callStmt,
                                    WPDSSolverTest::d_t srcNode,
                                    WPDSSolverTest::f_t destinationFunction,
                                    WPDSSolverTest::d_t destNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getReturnEdgeFunction(WPDSSolverTest::n_t callSite,
                                      WPDSSolverTest::f_t calleeFunction,
                                      WPDSSolverTest::n_t exitStmt,
                                      WPDSSolverTest::d_t exitNode,
                                      WPDSSolverTest::n_t reSite,
                                      WPDSSolverTest::d_t retNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getCallToRetEdgeFunction(WPDSSolverTest::n_t callSite,
                                         WPDSSolverTest::d_t callNode,
                                         WPDSSolverTest::n_t retSite,
                                         WPDSSolverTest::d_t retSiteNode,
                                         set<WPDSSolverTest::f_t> callees) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getSummaryEdgeFunction(WPDSSolverTest::n_t curr,
                                       WPDSSolverTest::d_t currNode,
                                       WPDSSolverTest::n_t succ,
                                       WPDSSolverTest::d_t succNode) {
  return nullptr;
}

WPDSSolverTest::l_t WPDSSolverTest::topElement() { return BinaryDomain::TOP; }

WPDSSolverTest::l_t WPDSSolverTest::bottomElement() {
  return BinaryDomain::BOTTOM;
}

WPDSSolverTest::l_t WPDSSolverTest::join(WPDSSolverTest::l_t lhs,
                                         WPDSSolverTest::l_t rhs) {
  return (lhs == BinaryDomain::BOTTOM || rhs == BinaryDomain::BOTTOM)
             ? BinaryDomain::BOTTOM
             : BinaryDomain::TOP;
}

WPDSSolverTest::d_t WPDSSolverTest::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool WPDSSolverTest::isZeroValue(WPDSSolverTest::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

std::map<WPDSSolverTest::n_t, std::set<WPDSSolverTest::d_t>>
WPDSSolverTest::initialSeeds() {
  return {{&ICF->getFunction("main")->front().front(), {getZeroValue()}}};
}

std::shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::allTopFunction() {
  return make_shared<AllTop<WPDSSolverTest::l_t>>(BinaryDomain::TOP);
}

void WPDSSolverTest::printNode(std::ostream &os, WPDSSolverTest::n_t n) const {
  os << llvmIRToString(n);
}

void WPDSSolverTest::printDataFlowFact(std::ostream &os,
                                       WPDSSolverTest::d_t d) const {
  os << llvmIRToString(d);
}

void WPDSSolverTest::printFunction(std::ostream &os,
                                   WPDSSolverTest::f_t m) const {
  os << m->getName().str();
}

void WPDSSolverTest::printEdgeFact(std::ostream &os,
                                   WPDSSolverTest::l_t v) const {
  if (v == BinaryDomain::TOP) {
    os << "TOP";
  } else {
    os << "BOTTOM";
  }
}

} // namespace psr
