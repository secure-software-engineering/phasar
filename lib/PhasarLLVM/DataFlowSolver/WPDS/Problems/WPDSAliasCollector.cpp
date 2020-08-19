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
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSAliasCollector.h"
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
                                       LLVMPointsToInfo *PT,
                                       std::set<std::string> EntryPoints)
    : WPDSProblem<WPDSAliasCollectorAnalysisDomain>(IRDB, TH, ICF, PT,
                                                    std::move(EntryPoints)) {}

WPDSAliasCollector::FlowFunctionPtrType
WPDSAliasCollector::getNormalFlowFunction(WPDSAliasCollector::n_t Curr,
                                          WPDSAliasCollector::n_t Succ) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

WPDSAliasCollector::FlowFunctionPtrType
WPDSAliasCollector::getCallFlowFunction(WPDSAliasCollector::n_t CallStmt,
                                        WPDSAliasCollector::f_t DestFun) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

WPDSAliasCollector::FlowFunctionPtrType WPDSAliasCollector::getRetFlowFunction(
    WPDSAliasCollector::n_t CallSite, WPDSAliasCollector::f_t CalleeFun,
    WPDSAliasCollector::n_t ExitStmt, WPDSAliasCollector::n_t RetSite) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

WPDSAliasCollector::FlowFunctionPtrType
WPDSAliasCollector::getCallToRetFlowFunction(
    WPDSAliasCollector::n_t CallSite, WPDSAliasCollector::n_t RetSite,
    set<WPDSAliasCollector::f_t> Callees) {
  return Identity<WPDSAliasCollector::d_t>::getInstance();
}

WPDSAliasCollector::FlowFunctionPtrType
WPDSAliasCollector::getSummaryFlowFunction(WPDSAliasCollector::n_t Curr,
                                           WPDSAliasCollector::f_t DestFun) {
  return nullptr;
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getNormalEdgeFunction(WPDSAliasCollector::n_t Curr,
                                          WPDSAliasCollector::d_t CurrNode,
                                          WPDSAliasCollector::n_t Succ,
                                          WPDSAliasCollector::d_t SuccNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getCallEdgeFunction(
    WPDSAliasCollector::n_t CallStmt, WPDSAliasCollector::d_t SrcNode,
    WPDSAliasCollector::f_t DestinationFunction,
    WPDSAliasCollector::d_t DestNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getReturnEdgeFunction(
    WPDSAliasCollector::n_t CallSite, WPDSAliasCollector::f_t CalleeFunction,
    WPDSAliasCollector::n_t ExitStmt, WPDSAliasCollector::d_t ExitNode,
    WPDSAliasCollector::n_t ReSite, WPDSAliasCollector::d_t RetNode) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getCallToRetEdgeFunction(
    WPDSAliasCollector::n_t CallSite, WPDSAliasCollector::d_t CallNode,
    WPDSAliasCollector::n_t RetSite, WPDSAliasCollector::d_t RetSiteNode,
    set<WPDSAliasCollector::f_t> Callees) {
  return EdgeIdentity<WPDSAliasCollector::l_t>::getInstance();
}

shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::getSummaryEdgeFunction(WPDSAliasCollector::n_t Curr,
                                           WPDSAliasCollector::d_t CurrNode,
                                           WPDSAliasCollector::n_t Succ,
                                           WPDSAliasCollector::d_t SuccNode) {
  return nullptr;
}

WPDSAliasCollector::l_t WPDSAliasCollector::topElement() {
  return BinaryDomain::TOP;
}
WPDSAliasCollector::l_t WPDSAliasCollector::bottomElement() {
  return BinaryDomain::BOTTOM;
}
WPDSAliasCollector::l_t WPDSAliasCollector::join(WPDSAliasCollector::l_t Lhs,
                                                 WPDSAliasCollector::l_t Rhs) {
  return (Lhs == BinaryDomain::BOTTOM || Rhs == BinaryDomain::BOTTOM)
             ? BinaryDomain::BOTTOM
             : BinaryDomain::TOP;
}

bool WPDSAliasCollector::isZeroValue(WPDSAliasCollector::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}
std::map<WPDSAliasCollector::n_t, std::set<WPDSAliasCollector::d_t>>
WPDSAliasCollector::initialSeeds() {
  return {};
}

std::shared_ptr<EdgeFunction<WPDSAliasCollector::l_t>>
WPDSAliasCollector::allTopFunction() {
  return make_shared<AllTop<WPDSAliasCollector::l_t>>(BinaryDomain::TOP);
}

void WPDSAliasCollector::printNode(std::ostream &OS,
                                   WPDSAliasCollector::n_t N) const {
  OS << llvmIRToString(N);
}

void WPDSAliasCollector::printDataFlowFact(std::ostream &OS,
                                           WPDSAliasCollector::d_t D) const {
  OS << llvmIRToString(D);
}

void WPDSAliasCollector::printFunction(std::ostream &OS,
                                       WPDSAliasCollector::f_t M) const {
  OS << M->getName().str();
}

void WPDSAliasCollector::printEdgeFact(std::ostream &OS,
                                       WPDSAliasCollector::l_t V) const {
  if (V == BinaryDomain::TOP) {
    OS << "TOP";
  } else {
    OS << "BOTTOM";
  }
}

} // namespace psr
