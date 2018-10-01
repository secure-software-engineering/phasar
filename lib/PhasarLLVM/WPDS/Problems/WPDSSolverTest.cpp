/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSSolverTest.h>

using namespace std;
using namespace psr;

namespace psr {

WPDSSolverTest::WPDSSolverTest(LLVMBasedICFG &I, WPDSType WPDS,
                               SearchDirection Direction,
                               std::vector<n_t> Stack, bool Witnesses)
    : WPDSProblem(I, WPDS, Direction, Stack, Witnesses) {}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getNormalFlowFunction(WPDSSolverTest::n_t curr,
                                      WPDSSolverTest::n_t succ) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getCallFlowFunction(WPDSSolverTest::n_t callStmt,
                                    WPDSSolverTest::m_t destMthd) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getRetFlowFunction(WPDSSolverTest::n_t callSite,
                                   WPDSSolverTest::m_t calleeMthd,
                                   WPDSSolverTest::n_t exitStmt,
                                   WPDSSolverTest::n_t retSite) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getCallToRetFlowFunction(WPDSSolverTest::n_t callSite,
                                         WPDSSolverTest::n_t retSite,
                                         set<WPDSSolverTest::m_t> callees) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<FlowFunction<WPDSSolverTest::d_t>>
WPDSSolverTest::getSummaryFlowFunction(WPDSSolverTest::n_t curr,
                                       WPDSSolverTest::m_t destMthd) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::v_t>>
WPDSSolverTest::getNormalEdgeFunction(WPDSSolverTest::n_t curr,
                                      WPDSSolverTest::d_t currNode,
                                      WPDSSolverTest::n_t succ,
                                      WPDSSolverTest::d_t succNode) {
  return EdgeIdentity<WPDSSolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::v_t>>
WPDSSolverTest::getCallEdgeFunction(WPDSSolverTest::n_t callStmt,
                                    WPDSSolverTest::d_t srcNode,
                                    WPDSSolverTest::m_t destiantionMethod,
                                    WPDSSolverTest::d_t destNode) {
  return EdgeIdentity<WPDSSolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::v_t>>
WPDSSolverTest::getReturnEdgeFunction(WPDSSolverTest::n_t callSite,
                                      WPDSSolverTest::m_t calleeMethod,
                                      WPDSSolverTest::n_t exitStmt,
                                      WPDSSolverTest::d_t exitNode,
                                      WPDSSolverTest::n_t reSite,
                                      WPDSSolverTest::d_t retNode) {
  return EdgeIdentity<WPDSSolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::v_t>>
WPDSSolverTest::getCallToRetEdgeFunction(WPDSSolverTest::n_t callSite,
                                         WPDSSolverTest::d_t callNode,
                                         WPDSSolverTest::n_t retSite,
                                         WPDSSolverTest::d_t retSiteNode,
                                         set<WPDSSolverTest::m_t> callees) {
  return EdgeIdentity<WPDSSolverTest::v_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::v_t>>
WPDSSolverTest::getSummaryEdgeFunction(WPDSSolverTest::n_t curr,
                                       WPDSSolverTest::d_t currNode,
                                       WPDSSolverTest::n_t succ,
                                       WPDSSolverTest::d_t succNode) {
  return EdgeIdentity<WPDSSolverTest::v_t>::getInstance();
}

WPDSSolverTest::v_t WPDSSolverTest::topElement() { return 1; }
WPDSSolverTest::v_t WPDSSolverTest::bottomElement() { return 0; }
WPDSSolverTest::v_t WPDSSolverTest::join(WPDSSolverTest::v_t lhs,
                                         WPDSSolverTest::v_t rhs) {
  return (lhs == true || rhs == true) ? true : 0;
}

WPDSSolverTest::d_t WPDSSolverTest::zeroValue() {
  return LLVMZeroValue::getInstance();
}

} // namespace psr
