/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <array>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Kill.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {
IDESecureHeapPropagation::IDESecureHeapPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  ZeroValue = createZeroValue();
}

std::shared_ptr<FlowFunction<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::getNormalFlowFunction(n_t curr, n_t succ) {
  return Identity<d_t>::getInstance();
}

std::shared_ptr<FlowFunction<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::getCallFlowFunction(n_t callStmt, m_t destMthd) {
  return Identity<d_t>::getInstance();
}

std::shared_ptr<FlowFunction<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::getRetFlowFunction(n_t callSite, m_t calleeMthd,
                                             n_t exitStmt, n_t retSite) {
  return Identity<d_t>::getInstance();
}

std::shared_ptr<FlowFunction<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                                   std::set<m_t> callees) {

  llvm::ImmutableCallSite CS(callSite);

  auto fName = CS.getCalledFunction()->getName();
  if (fName == initializerFn) {
    return std::make_shared<Gen<d_t>>(SecureHeapFact::INITIALIZED,
                                      getZeroValue());
  }
  return Identity<d_t>::getInstance();
}
std::shared_ptr<FlowFunction<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::getSummaryFlowFunction(n_t callStmt, m_t destMthd) {
  return nullptr;
}

std::map<IDESecureHeapPropagation::n_t, std::set<IDESecureHeapPropagation::d_t>>
IDESecureHeapPropagation::initialSeeds() {
  std::map<n_t, std::set<d_t>> seeds;
  for (auto &entry : EntryPoints) {
    auto fn = ICF->getFunction(entry);
    if (fn && !fn->isDeclaration())
      seeds[&fn->front().front()] = {getZeroValue()};
  }
  return seeds;
}

IDESecureHeapPropagation::d_t
IDESecureHeapPropagation::createZeroValue() const {
  return SecureHeapFact::ZERO;
}

bool IDESecureHeapPropagation::isZeroValue(d_t d) const {
  return d == SecureHeapFact::ZERO;
}

void IDESecureHeapPropagation::printNode(std::ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IDESecureHeapPropagation::printDataFlowFact(std::ostream &os,
                                                 d_t d) const {
  switch (d) {
  case SecureHeapFact::ZERO:
    os << "ZERO";
    break;
  case SecureHeapFact::INITIALIZED:
    os << "INITIALIZED";
    break;
  default:
    assert(false && "Invalid dataflow-fact");
    break;
  }
}

void IDESecureHeapPropagation::printFunction(std::ostream &os, m_t m) const {
  os << cxx_demangle(m->getName().str());
}

// in addition provide specifications for the IDE parts

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getNormalEdgeFunction(n_t curr, d_t currNode,
                                                n_t succ, d_t succNode) {
  return IdentityEdgeFunction::getInstance();
}
std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getCallEdgeFunction(n_t callStmt, d_t srcNode,
                                              m_t destinationMethod,
                                              d_t destNode) {
  return IdentityEdgeFunction::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getReturnEdgeFunction(n_t callSite, m_t calleeMethod,
                                                n_t exitStmt, d_t exitNode,
                                                n_t reSite, d_t retNode) {
  return IdentityEdgeFunction::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getCallToRetEdgeFunction(n_t callSite, d_t callNode,
                                                   n_t retSite, d_t retSiteNode,
                                                   std::set<m_t> callees) {
  if (callNode == ZeroValue && retSiteNode != ZeroValue) {
    // generate
    // std::cerr << "Generate at " << llvmIRToShortString(callSite) <<
    // std::endl;
    return SHPGenEdgeFn::getInstance(l_t::INITIALIZED);
  }
  llvm::ImmutableCallSite CS(callSite);
  if (callNode != ZeroValue &&
      CS.getCalledFunction()->getName() == shutdownFn) {
    // std::cerr << "Kill at " << llvmIRToShortString(callSite) << std::endl;
    return SHPGenEdgeFn::getInstance(l_t::BOT);
  }
  return IdentityEdgeFunction::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getSummaryEdgeFunction(n_t callStmt, d_t callNode,
                                                 n_t retSite, d_t retSiteNode) {
  return nullptr;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::topElement() {
  return l_t::TOP;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::bottomElement() {
  return l_t::BOT;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::join(l_t lhs, l_t rhs) {
  if (lhs == rhs)
    return lhs;
  if (lhs == l_t::TOP)
    return rhs;
  if (rhs == l_t::TOP)
    return lhs;
  return l_t::BOT;
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::allTopFunction() {
  static auto allTop = std::make_shared<AllTop<l_t>>(l_t::TOP);
  return allTop;
}

void IDESecureHeapPropagation::printEdgeFact(std::ostream &os, l_t l) const {
  switch (l) {
  case l_t::BOT:
    os << "BOT";
    break;
  case l_t::INITIALIZED:
    os << "INITIALIZED";
    break;
  case l_t::TOP:
    os << "TOP";
    break;
  default:
    assert(false && "Invalid edge fact");
    break;
  }
}

void IDESecureHeapPropagation::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, std::ostream &os) {
  for (auto f : ICF->getAllFunctions()) {
    std::string fName = getFunctionNameFromIR(f);
    os << "\nFunction: " << fName << "\n----------"
       << std::string(fName.size(), '-') << '\n';
    for (auto stmt : ICF->getAllInstructionsOf(f)) {
      auto results = SR.resultsAt(stmt, true);

      if (!results.empty()) {
        os << "At IR statement: " << NtoString(stmt) << '\n';
        for (auto res : results) {

          os << "   Fact: " << DtoString(res.first)
             << "\n  Value: " << LtoString(res.second) << '\n';
        }
        os << '\n';
      }
    }
    os << '\n';
  }
}

// -- IdentityEdgeFunction --
IDESecureHeapPropagation::l_t
IDESecureHeapPropagation::IdentityEdgeFunction::computeTarget(l_t source) {
  return source;
}

bool IDESecureHeapPropagation::IdentityEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> other) const {
  return other.get() == this; // singelton
}

void IDESecureHeapPropagation::IdentityEdgeFunction::print(
    std::ostream &OS, bool isForDebug) const {
  OS << "IdentityEdgeFn";
}
std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::IdentityEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> secondFunction) {
  if (dynamic_cast<AllBottom<l_t> *>(secondFunction.get()))
    return shared_from_this();
  return secondFunction;
}
std::shared_ptr<IDESecureHeapPropagation::IdentityEdgeFunction>
IDESecureHeapPropagation::IdentityEdgeFunction::getInstance() {
  static auto cache = std::make_shared<IdentityEdgeFunction>();
  return cache;
}
// -- SHPEdgeFn --
std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::SHPEdgeFn::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> otherFunction) {

  if (otherFunction.get() != this)
    return SHPGenEdgeFn::getInstance(l_t::BOT);
  return shared_from_this();
}

// -- SHPGenEdgeFn --

IDESecureHeapPropagation::SHPGenEdgeFn::SHPGenEdgeFn(l_t val) : value(val) {}

IDESecureHeapPropagation::l_t
IDESecureHeapPropagation::SHPGenEdgeFn::computeTarget(l_t source) {
  return value;
}
std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::SHPGenEdgeFn::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> secondFunction) {
  return SHPGenEdgeFn::getInstance(secondFunction->computeTarget(value));
}
bool IDESecureHeapPropagation::SHPGenEdgeFn::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> other) const {
  return other.get() == this; // reference-equality
}

void IDESecureHeapPropagation::SHPGenEdgeFn::print(std::ostream &OS,
                                                   bool isForDebug) const {
  OS << "GenEdgeFn[";
  switch (value) {
  case l_t::BOT:
    OS << "BOT";
    break;
  case l_t::INITIALIZED:
    OS << "INITIALIZED";
    break;
  case l_t::TOP:
    OS << "TOP";
    break;
  default:
    assert(false && "Invalid edge value");
    break;
  }
  OS << "]";
}
std::shared_ptr<IDESecureHeapPropagation::SHPGenEdgeFn>
IDESecureHeapPropagation::SHPGenEdgeFn::getInstance(l_t val) {
  static std::array<std::shared_ptr<SHPGenEdgeFn>, 3> cache = {nullptr, nullptr,
                                                               nullptr};
  auto ind = static_cast<std::underlying_type<l_t>::type>(val);
  if (!cache[ind]) {
    cache[ind] = std::make_shared<SHPGenEdgeFn>(val);
  }
  return cache[ind];
}
} // namespace psr