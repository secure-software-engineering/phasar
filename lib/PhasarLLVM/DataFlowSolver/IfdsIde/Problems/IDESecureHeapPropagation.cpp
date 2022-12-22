/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <array>
#include <utility>

namespace psr {

struct SHPEdgeFn
    : public EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>,
      public std::enable_shared_from_this<SHPEdgeFn> {
  ~SHPEdgeFn() override = default;

  std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
  joinWith(
      std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
          OtherFunction) override;
};

struct SHPEdgeFunctionComposer
    : public EdgeFunctionComposer<IDESecureHeapPropagationAnalysisDomain::l_t>,
      public std::enable_shared_from_this<SHPEdgeFn> {
  ~SHPEdgeFunctionComposer() override = default;
  std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
  joinWith(
      std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
          OtherFunction) override;
};
struct SHPGenEdgeFn : public SHPEdgeFn {
  using l_t = IDESecureHeapPropagationAnalysisDomain::l_t;
  SHPGenEdgeFn(l_t Val) : Value(Val) {}
  ~SHPGenEdgeFn() override = default;

  l_t computeTarget(l_t /*Source*/) override { return Value; }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> SecondFunction) override {
    return SHPGenEdgeFn::getInstance(SecondFunction->computeTarget(Value));
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> Other) const override {
    return Other.get() == this; // reference-equality
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug*/ = false) const override {
    OS << "GenEdgeFn[";
    switch (Value) {
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

  static std::shared_ptr<SHPGenEdgeFn> getInstance(l_t Val) {
    static std::array<std::shared_ptr<SHPGenEdgeFn>, 3> Cache = {
        nullptr, nullptr, nullptr};
    auto Ind = static_cast<std::underlying_type<l_t>::type>(Val);
    if (!Cache.at(Ind)) {
      Cache[Ind] = std::make_shared<SHPGenEdgeFn>(Val);
    }
    return Cache.at(Ind);
  }

private:
  l_t Value;
};

// -- SHPEdgeFn --
std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
SHPEdgeFn::joinWith(
    std::shared_ptr<EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>>
        OtherFunction) {

  if (OtherFunction.get() != this) {
    return SHPGenEdgeFn::getInstance(
        IDESecureHeapPropagationAnalysisDomain::l_t::BOT);
  }
  return shared_from_this();
}

IDESecureHeapPropagation::IDESecureHeapPropagation(
    const LLVMProjectIRDB *IRDB, std::vector<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getNormalFlowFunction(n_t /*Curr*/, n_t /*Succ*/) {
  return Identity<d_t>::getInstance();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getCallFlowFunction(n_t /*CallSite*/,
                                              f_t /*DestMthd*/) {
  return Identity<d_t>::getInstance();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getRetFlowFunction(n_t /*CallSite*/,
                                             f_t /*CalleeMthd*/,
                                             n_t /*ExitInst*/,
                                             n_t /*RetSite*/) {
  return Identity<d_t>::getInstance();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> /*Callees*/) {

  // Change to CallSite everywhere
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

  auto FName = CS->getCalledFunction()->getName();
  if (FName == InitializerFn) {
    return std::make_shared<Gen<d_t>>(SecureHeapFact::INITIALIZED,
                                      getZeroValue());
  }
  return Identity<d_t>::getInstance();
}
IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getSummaryFlowFunction(n_t /*CallSite*/,
                                                 f_t /*DestMthd*/) {
  return nullptr;
}

InitialSeeds<IDESecureHeapPropagation::n_t, IDESecureHeapPropagation::d_t,
             IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::initialSeeds() {
  InitialSeeds<IDESecureHeapPropagation::n_t, IDESecureHeapPropagation::d_t,
               IDESecureHeapPropagation::l_t>
      Seeds;
  for (const auto &Entry : EntryPoints) {
    const auto *Fn = IRDB->getFunction(Entry);
    if (Fn && !Fn->isDeclaration()) {
      Seeds.addSeed(&Fn->front().front(), getZeroValue(), bottomElement());
    }
  }
  return Seeds;
}

IDESecureHeapPropagation::d_t
IDESecureHeapPropagation::createZeroValue() const {
  return SecureHeapFact::ZERO;
}

bool IDESecureHeapPropagation::isZeroValue(d_t Fact) const {
  return Fact == SecureHeapFact::ZERO;
}

void IDESecureHeapPropagation::printNode(llvm::raw_ostream &Os,
                                         n_t Stmt) const {
  Os << llvmIRToString(Stmt);
}

void IDESecureHeapPropagation::printDataFlowFact(llvm::raw_ostream &Os,
                                                 d_t Fact) const {
  switch (Fact) {
  case SecureHeapFact::ZERO:
    Os << "ZERO";
    break;
  case SecureHeapFact::INITIALIZED:
    Os << "INITIALIZED";
    break;
  default:
    assert(false && "Invalid dataflow-fact");
    break;
  }
}

void IDESecureHeapPropagation::printFunction(llvm::raw_ostream &Os,
                                             f_t F) const {
  Os << llvm::demangle(F->getName().str());
}

// in addition provide specifications for the IDE parts

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                                n_t /*Succ*/,
                                                d_t /*SuccNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}
std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getCallEdgeFunction(n_t /*CallSite*/, d_t /*SrcNode*/,
                                              f_t /*DestinationMethod*/,
                                              d_t /*DestNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeMethod*/, n_t /*ExitInst*/, d_t /*ExitNode*/,
    n_t /*RetSite*/, d_t /*RetNode*/) {
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t /*RetSite*/, d_t RetSiteNode,
    llvm::ArrayRef<f_t> /*Callees*/) {
  if (CallNode == ZeroValue && RetSiteNode != ZeroValue) {
    // generate
    // std::cerr << "Generate at " << llvmIRToShortString(callSite) <<
    // std::endl;
    return SHPGenEdgeFn::getInstance(l_t::INITIALIZED);
  }
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  if (CallNode != ZeroValue &&
      CS->getCalledFunction()->getName() == ShutdownFn) {
    // std::cerr << "Kill at " << llvmIRToShortString(callSite) << std::endl;
    return SHPGenEdgeFn::getInstance(l_t::BOT);
  }
  return EdgeIdentity<l_t>::getInstance();
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::getSummaryEdgeFunction(n_t /*CallSite*/,
                                                 d_t /*CallNode*/,
                                                 n_t /*RetSite*/,
                                                 d_t /*RetSiteNode*/) {
  return nullptr;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::topElement() {
  return l_t::TOP;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::bottomElement() {
  return l_t::BOT;
}

IDESecureHeapPropagation::l_t IDESecureHeapPropagation::join(l_t Lhs, l_t Rhs) {
  if (Lhs == Rhs) {
    return Lhs;
  }
  if (Lhs == l_t::TOP) {
    return Rhs;
  }
  if (Rhs == l_t::TOP) {
    return Lhs;
  }
  return l_t::BOT;
}

std::shared_ptr<EdgeFunction<IDESecureHeapPropagation::l_t>>
IDESecureHeapPropagation::allTopFunction() {
  static auto AT = std::make_shared<AllTop<l_t>>(l_t::TOP);
  return AT;
}

void IDESecureHeapPropagation::printEdgeFact(llvm::raw_ostream &Os,
                                             l_t L) const {
  switch (L) {
  case l_t::BOT:
    Os << "BOT";
    break;
  case l_t::INITIALIZED:
    Os << "INITIALIZED";
    break;
  case l_t::TOP:
    Os << "TOP";
    break;
  default:
    assert(false && "Invalid edge fact");
    break;
  }
}

void IDESecureHeapPropagation::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, llvm::raw_ostream &Os) {
  LLVMBasedCFG CFG;

  for (const auto *F : IRDB->getAllFunctions()) {
    std::string FName = getFunctionNameFromIR(F);
    Os << "\nFunction: " << FName << "\n----------"
       << std::string(FName.size(), '-') << '\n';
    for (const auto *Stmt : CFG.getAllInstructionsOf(F)) {
      auto Results = SR.resultsAt(Stmt, true);

      if (!Results.empty()) {
        Os << "At IR statement: " << NtoString(Stmt) << '\n';
        for (auto Res : Results) {

          Os << "   Fact: " << DtoString(Res.first)
             << "\n  Value: " << LtoString(Res.second) << '\n';
        }
        Os << '\n';
      }
    }
    Os << '\n';
  }
}

} // namespace psr
