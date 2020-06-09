/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

WPDSSolverTest::WPDSSolverTest(const ProjectIRDB *IRDB,
                               const LLVMTypeHierarchy *TH,
                               const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                               std::set<std::string> EntryPoints)
    : WPDSProblem<WPDSSolverTestAnalysisDomain>(IRDB, TH, ICF, PT,
                                                std::move(EntryPoints)) {}

WPDSSolverTest::FlowFunctionPtrType
WPDSSolverTest::getNormalFlowFunction(WPDSSolverTest::n_t Curr,
                                      WPDSSolverTest::n_t Succ) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

WPDSSolverTest::FlowFunctionPtrType
WPDSSolverTest::getCallFlowFunction(WPDSSolverTest::n_t CallStmt,
                                    WPDSSolverTest::f_t DestFun) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

WPDSSolverTest::FlowFunctionPtrType WPDSSolverTest::getRetFlowFunction(
    WPDSSolverTest::n_t CallSite, WPDSSolverTest::f_t CalleeFun,
    WPDSSolverTest::n_t ExitStmt, WPDSSolverTest::n_t RetSite) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

WPDSSolverTest::FlowFunctionPtrType
WPDSSolverTest::getCallToRetFlowFunction(WPDSSolverTest::n_t CallSite,
                                         WPDSSolverTest::n_t RetSite,
                                         set<WPDSSolverTest::f_t> Callees) {
  return Identity<WPDSSolverTest::d_t>::getInstance();
}

WPDSSolverTest::FlowFunctionPtrType
WPDSSolverTest::getSummaryFlowFunction(WPDSSolverTest::n_t Curr,
                                       WPDSSolverTest::f_t DestFun) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getNormalEdgeFunction(WPDSSolverTest::n_t Curr,
                                      WPDSSolverTest::d_t CurrNode,
                                      WPDSSolverTest::n_t Succ,
                                      WPDSSolverTest::d_t SuccNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getCallEdgeFunction(WPDSSolverTest::n_t CallStmt,
                                    WPDSSolverTest::d_t SrcNode,
                                    WPDSSolverTest::f_t DestinationFunction,
                                    WPDSSolverTest::d_t DestNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getReturnEdgeFunction(WPDSSolverTest::n_t CallSite,
                                      WPDSSolverTest::f_t CalleeFunction,
                                      WPDSSolverTest::n_t ExitStmt,
                                      WPDSSolverTest::d_t ExitNode,
                                      WPDSSolverTest::n_t ReSite,
                                      WPDSSolverTest::d_t RetNode) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getCallToRetEdgeFunction(WPDSSolverTest::n_t CallSite,
                                         WPDSSolverTest::d_t CallNode,
                                         WPDSSolverTest::n_t RetSite,
                                         WPDSSolverTest::d_t RetSiteNode,
                                         set<WPDSSolverTest::f_t> Callees) {
  return EdgeIdentity<WPDSSolverTest::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::getSummaryEdgeFunction(WPDSSolverTest::n_t Curr,
                                       WPDSSolverTest::d_t CurrNode,
                                       WPDSSolverTest::n_t Succ,
                                       WPDSSolverTest::d_t SuccNode) {
  return nullptr;
}

WPDSSolverTest::l_t WPDSSolverTest::topElement() { return BinaryDomain::TOP; }

WPDSSolverTest::l_t WPDSSolverTest::bottomElement() {
  return BinaryDomain::BOTTOM;
}

WPDSSolverTest::l_t WPDSSolverTest::join(WPDSSolverTest::l_t Lhs,
                                         WPDSSolverTest::l_t Rhs) {
  return (Lhs == BinaryDomain::BOTTOM || Rhs == BinaryDomain::BOTTOM)
             ? BinaryDomain::BOTTOM
             : BinaryDomain::TOP;
}

WPDSSolverTest::d_t WPDSSolverTest::createZeroValue() const {
  return LLVMZeroValue::getInstance();
}

bool WPDSSolverTest::isZeroValue(WPDSSolverTest::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

std::map<WPDSSolverTest::n_t, std::set<WPDSSolverTest::d_t>>
WPDSSolverTest::initialSeeds() {
  return {{&ICF->getFunction("main")->front().front(), {getZeroValue()}}};
}

std::shared_ptr<EdgeFunction<WPDSSolverTest::l_t>>
WPDSSolverTest::allTopFunction() {
  return make_shared<AllTop<WPDSSolverTest::l_t>>(BinaryDomain::TOP);
}

void WPDSSolverTest::printNode(std::ostream &OS, WPDSSolverTest::n_t N) const {
  OS << llvmIRToString(N);
}

void WPDSSolverTest::printDataFlowFact(std::ostream &OS,
                                       WPDSSolverTest::d_t D) const {
  OS << llvmIRToString(D);
}

void WPDSSolverTest::printFunction(std::ostream &OS,
                                   WPDSSolverTest::f_t M) const {
  OS << M->getName().str();
}

void WPDSSolverTest::printEdgeFact(std::ostream &OS,
                                   WPDSSolverTest::l_t V) const {
  if (V == BinaryDomain::TOP) {
    OS << "TOP";
  } else {
    OS << "BOTTOM";
  }
}

} // namespace psr
