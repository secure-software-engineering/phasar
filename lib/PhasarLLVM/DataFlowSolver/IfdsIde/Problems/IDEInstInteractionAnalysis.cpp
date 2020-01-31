/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Utilities.h>

using namespace psr;
using namespace std;

namespace psr {

IDEInstInteractionAnalysis::IDEInstInteractionAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

void IDEInstInteractionAnalysis::registerEdgeFactGenerator(
    std::function<std::set<IDEInstInteractionAnalysis::v_t>(
        IDEInstInteractionAnalysis::n_t curr,
        IDEInstInteractionAnalysis::d_t srcNode,
        IDEInstInteractionAnalysis::d_t destNode)>
        EdgeFactGenerator) {
  EdgeFactGen = EdgeFactGenerator;
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::getNormalFlowFunction(
    IDEInstInteractionAnalysis::n_t curr,
    IDEInstInteractionAnalysis::n_t succ) {

  struct IIAFlowFunction : FlowFunction<IDEInstInteractionAnalysis::d_t> {

    IDEInstInteractionAnalysis &Problem;
    IDEInstInteractionAnalysis::n_t Inst;

    IIAFlowFunction(IDEInstInteractionAnalysis &Problem,
                    IDEInstInteractionAnalysis::n_t Inst)
        : Problem(Problem), Inst(Inst) {}

    std::set<IDEInstInteractionAnalysis::d_t>
    computeTargets(IDEInstInteractionAnalysis::d_t src) override {
      std::set<IDEInstInteractionAnalysis::d_t> Facts;
      if (Problem.isZeroValue(src)) {
        // keep the zero flow fact
        Facts.insert(src);
        return Facts;
      }
      // populate and propagate other existing facts
      for (auto &Op : Inst->operands()) {
        // if one of the operands holds, also generate the instruction using it
        if (Op == src) {
          Facts.insert(Inst);
        }
      }
      // pass everything that alreay holds as identity
      Facts.insert(src);
      return Facts;
    }
  };
  return std::make_shared<IIAFlowFunction>(*this, curr);
}

shared_ptr<FlowFunction<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::getCallFlowFunction(
    IDEInstInteractionAnalysis::n_t callStmt,
    IDEInstInteractionAnalysis::m_t destMthd) {
  return std::make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                            destMthd);
}

shared_ptr<FlowFunction<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::getRetFlowFunction(
    IDEInstInteractionAnalysis::n_t callSite,
    IDEInstInteractionAnalysis::m_t calleeMthd,
    IDEInstInteractionAnalysis::n_t exitStmt,
    IDEInstInteractionAnalysis::n_t retSite) {
  return std::make_shared<MapFactsToCaller>(llvm::ImmutableCallSite(callSite),
                                            calleeMthd, exitStmt);
}

shared_ptr<FlowFunction<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::getCallToRetFlowFunction(
    IDEInstInteractionAnalysis::n_t callSite,
    IDEInstInteractionAnalysis::n_t retSite,
    set<IDEInstInteractionAnalysis::m_t> callees) {
  return Identity<IDEInstInteractionAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::getSummaryFlowFunction(
    IDEInstInteractionAnalysis::n_t callStmt,
    IDEInstInteractionAnalysis::m_t destMthd) {
  // do not use summaries
  return nullptr;
}

map<IDEInstInteractionAnalysis::n_t, set<IDEInstInteractionAnalysis::d_t>>
IDEInstInteractionAnalysis::initialSeeds() {
  cout << "IDEInstInteractionAnalysis::initialSeeds()\n";
  map<IDEInstInteractionAnalysis::n_t, set<IDEInstInteractionAnalysis::d_t>>
      SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(
        make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                  set<IDEInstInteractionAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDEInstInteractionAnalysis::d_t
IDEInstInteractionAnalysis::createZeroValue() const {
  cout << "IDEInstInteractionAnalysis::createZeroValue()\n";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDEInstInteractionAnalysis::isZeroValue(
    IDEInstInteractionAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::getNormalEdgeFunction(
    IDEInstInteractionAnalysis::n_t curr,
    IDEInstInteractionAnalysis::d_t currNode,
    IDEInstInteractionAnalysis::n_t succ,
    IDEInstInteractionAnalysis::d_t succNode) {
  // check if the user has registered a fact generator function
  std::set<IDEInstInteractionAnalysis::v_t> UserEdgeFacts;
  if (EdgeFactGen) {
    UserEdgeFacts = EdgeFactGen(curr, currNode, succNode);
  }
  // In addition to the edge facts generated by the ordinary edge functions,
  // generate the UserEdgeFacts, too.
  return EdgeIdentity<IDEInstInteractionAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::getCallEdgeFunction(
    IDEInstInteractionAnalysis::n_t callStmt,
    IDEInstInteractionAnalysis::d_t srcNode,
    IDEInstInteractionAnalysis::m_t destinationMethod,
    IDEInstInteractionAnalysis::d_t destNode) {
  // can be passed as identity
  return EdgeIdentity<IDEInstInteractionAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::getReturnEdgeFunction(
    IDEInstInteractionAnalysis::n_t callSite,
    IDEInstInteractionAnalysis::m_t calleeMethod,
    IDEInstInteractionAnalysis::n_t exitStmt,
    IDEInstInteractionAnalysis::d_t exitNode,
    IDEInstInteractionAnalysis::n_t reSite,
    IDEInstInteractionAnalysis::d_t retNode) {
  // can be passed as identity
  return EdgeIdentity<IDEInstInteractionAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::getCallToRetEdgeFunction(
    IDEInstInteractionAnalysis::n_t callSite,
    IDEInstInteractionAnalysis::d_t callNode,
    IDEInstInteractionAnalysis::n_t retSite,
    IDEInstInteractionAnalysis::d_t retSiteNode,
    set<IDEInstInteractionAnalysis::m_t> callees) {
  return EdgeIdentity<IDEInstInteractionAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::getSummaryEdgeFunction(
    IDEInstInteractionAnalysis::n_t callSite,
    IDEInstInteractionAnalysis::d_t callNode,
    IDEInstInteractionAnalysis::n_t retSite,
    IDEInstInteractionAnalysis::d_t retSiteNode) {
  // do not use summaries
  return nullptr;
}

IDEInstInteractionAnalysis::l_t IDEInstInteractionAnalysis::topElement() {
  cout << "IDEInstInteractionAnalysis::topElement()\n";
  // have empty set to represent no information
  return {};
}

IDEInstInteractionAnalysis::l_t IDEInstInteractionAnalysis::bottomElement() {
  cout << "IDEInstInteractionAnalysis::bottomElement()\n";
  return {};
}

IDEInstInteractionAnalysis::l_t
IDEInstInteractionAnalysis::join(IDEInstInteractionAnalysis::l_t lhs,
                                 IDEInstInteractionAnalysis::l_t rhs) {
  cout << "IDEInstInteractionAnalysis::join()\n";
  return lhs.setUnion(rhs);
}

shared_ptr<EdgeFunction<IDEInstInteractionAnalysis::l_t>>
IDEInstInteractionAnalysis::allTopFunction() {
  cout << "IDEInstInteractionAnalysis::allTopFunction()\n";
  return make_shared<AllTop<IDEInstInteractionAnalysis::l_t>>(topElement());
}

void IDEInstInteractionAnalysis::printNode(
    ostream &os, IDEInstInteractionAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IDEInstInteractionAnalysis::printDataFlowFact(
    ostream &os, IDEInstInteractionAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IDEInstInteractionAnalysis::printFunction(
    ostream &os, IDEInstInteractionAnalysis::m_t m) const {
  os << m->getName().str();
}

void IDEInstInteractionAnalysis::printEdgeFact(
    ostream &os, IDEInstInteractionAnalysis::l_t l) const {
  auto lset = l.getAsSet();
  for (const auto &s : lset) {
    os << s;
    if (s == *lset.rbegin()) {
      os << ", ";
    }
  }
}

} // namespace psr
