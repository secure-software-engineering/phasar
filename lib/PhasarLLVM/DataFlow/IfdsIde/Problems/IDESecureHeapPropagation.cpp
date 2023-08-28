/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESecureHeapPropagation.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"

#include <array>
#include <utility>

namespace psr {

struct SHPEdgeFn {
  template <typename ConcreteEF>
  static EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>
  join(EdgeFunctionRef<ConcreteEF> This,
       const EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>
           &OtherFunction);
};

struct SHPEdgeFunctionComposer
    : EdgeFunctionComposer<IDESecureHeapPropagationAnalysisDomain::l_t> {

  static EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>
  join(EdgeFunctionRef<SHPEdgeFunctionComposer> This,
       EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t> OtherFunction);
};
using SHPGenEdgeFn =
    ConstantEdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>;

// -- SHPEdgeFn --
template <typename ConcreteEF>
EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>
SHPEdgeFn::join(EdgeFunctionRef<ConcreteEF> This,
                const EdgeFunction<IDESecureHeapPropagationAnalysisDomain::l_t>
                    &OtherFunction) {
  if (OtherFunction != This) {
    return SHPGenEdgeFn{IDESecureHeapPropagationAnalysisDomain::l_t::BOT};
    /// TODO: Why not AllBottom?
  }
  return This;
}

IDESecureHeapPropagation::IDESecureHeapPropagation(
    const LLVMProjectIRDB *IRDB, std::vector<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()) {}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getNormalFlowFunction(n_t /*Curr*/, n_t /*Succ*/) {
  return identityFlow();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getCallFlowFunction(n_t /*CallSite*/,
                                              f_t /*DestMthd*/) {
  return identityFlow();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getRetFlowFunction(n_t /*CallSite*/,
                                             f_t /*CalleeMthd*/,
                                             n_t /*ExitInst*/,
                                             n_t /*RetSite*/) {
  return identityFlow();
}

IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> /*Callees*/) {

  // Change to CallSite everywhere
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

  auto FName = CS->getCalledFunction()->getName();
  if (FName == InitializerFn) {
    return generateFromZero(SecureHeapFact::INITIALIZED);
  }
  return identityFlow();
}
IDESecureHeapPropagation::FlowFunctionPtrType
IDESecureHeapPropagation::getSummaryFlowFunction(n_t /*CallSite*/,
                                                 f_t /*DestMthd*/) {
  return nullptr;
}

InitialSeeds<IDESecureHeapPropagation::n_t, IDESecureHeapPropagation::d_t,
             IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::initialSeeds() {
  return createDefaultSeeds();
}

IDESecureHeapPropagation::d_t
IDESecureHeapPropagation::createZeroValue() const {
  return SecureHeapFact::ZERO;
}

bool IDESecureHeapPropagation::isZeroValue(d_t Fact) const noexcept {
  return Fact == SecureHeapFact::ZERO;
}

// in addition provide specifications for the IDE parts

EdgeFunction<IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                                n_t /*Succ*/,
                                                d_t /*SuccNode*/) {
  return EdgeIdentity<l_t>{};
}
EdgeFunction<IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::getCallEdgeFunction(n_t /*CallSite*/, d_t /*SrcNode*/,
                                              f_t /*DestinationMethod*/,
                                              d_t /*DestNode*/) {
  return EdgeIdentity<l_t>{};
}

EdgeFunction<IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeMethod*/, n_t /*ExitInst*/, d_t /*ExitNode*/,
    n_t /*RetSite*/, d_t /*RetNode*/) {
  return EdgeIdentity<l_t>{};
}

EdgeFunction<IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t /*RetSite*/, d_t RetSiteNode,
    llvm::ArrayRef<f_t> /*Callees*/) {
  if (CallNode == ZeroValue && RetSiteNode != ZeroValue) {
    // generate
    // std::cerr << "Generate at " << llvmIRToShortString(callSite) <<
    // std::endl;
    return SHPGenEdgeFn{l_t::INITIALIZED};
  }
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  if (CallNode != ZeroValue &&
      CS->getCalledFunction()->getName() == ShutdownFn) {
    // std::cerr << "Kill at " << llvmIRToShortString(callSite) << std::endl;
    return SHPGenEdgeFn{l_t::BOT};
  }
  return EdgeIdentity<l_t>{};
}

EdgeFunction<IDESecureHeapPropagation::l_t>
IDESecureHeapPropagation::getSummaryEdgeFunction(n_t /*CallSite*/,
                                                 d_t /*CallNode*/,
                                                 n_t /*RetSite*/,
                                                 d_t /*RetSiteNode*/) {
  return nullptr;
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
        Os << "At IR statement: " << NToString(Stmt) << '\n';
        for (auto Res : Results) {

          Os << "   Fact: " << DToString(Res.first)
             << "\n  Value: " << LToString(Res.second) << '\n';
        }
        Os << '\n';
      }
    }
    Os << '\n';
  }
}

} // namespace psr

llvm::StringRef psr::DToString(SecureHeapFact Fact) noexcept {
  switch (Fact) {
  case SecureHeapFact::ZERO:
    return "ZERO";
  case SecureHeapFact::INITIALIZED:
    return "INITIALIZED";
  }
  llvm_unreachable("Invalid dataflow-fact");
}

llvm::StringRef psr::LToString(SecureHeapValue Val) noexcept {
  switch (Val) {
  case SecureHeapValue::BOT:
    return "BOT";
  case SecureHeapValue::INITIALIZED:
    return "INITIALIZED";
  case SecureHeapValue::TOP:
    return "TOP";
  }
  llvm_unreachable("Invalid edge fact");
}
