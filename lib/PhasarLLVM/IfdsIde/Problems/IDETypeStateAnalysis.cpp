/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETypeStateAnalysis.h>

#include <phasar/Utils/LLVMShorthands.h>
//#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

const State IDETypeStateAnalysis::TOP = uninit;

const State open = opened;
const State close = closed;

const State IDETypeStateAnalysis::BOTTOM = error;

IDETypeStateAnalysis::IDETypeStateAnalysis(IDETypeStateAnalysis::i_t icfg,
                                           vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t curr,
                                            IDETypeStateAnalysis::n_t succ) {
  cout << "Once: " << curr << endl;
  // check alloca instruction for file handler
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Alloca,
                                                             zeroValue());
        }
      }
    }
    // struct UnsereFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
    //   const llvm::AllocaInst *Alloc;
    //   const llvm::Value *ZV;
    //   UnsereFlowFunction(const llvm::AllocaInst *A, const llvm::Value *Z)
    //       : Alloc(A), ZV(Z) {}

    //   set<IDETypeStateAnalysis::d_t>
    //   computeTargets(IDETypeStateAnalysis::d_t source) override {
    //     if (source == ZV) {
    //       return {source, Alloc};
    //     } else {
    //       return {source};
    //     }
    //   }
    // };
    // return make_shared<UnsereFlowFunction>(Alloca, zeroValue());
    // return make_shared<Lambda<IDETypeStateAnalysis::d_t>>([Variablen der
    // "Außenwelt können hier gecaptured werden"](IDETypeStateAnalysis::d_t
    // source) {

    //   return set<IDETypeStateAnalysis::d_t>{};
    // });
  }

  // check load instructions for file handler
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    /**/ if (Load->getPointerOperand()
                 ->getType()
                 ->getPointerElementType()
                 ->isPointerTy()) {
      if (auto StructTy =
              llvm::dyn_cast<llvm::StructType>(Load->getPointerOperand()
                                                   ->getType()
                                                   ->getPointerElementType()
                                                   ->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Load, zeroValue());
        }
      }
    /**/}
  }

  // check store instructions for file handler
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Store->getValueOperand()->getType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Store,
                                                             zeroValue());
        }
      }
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t callStmt,
                                          IDETypeStateAnalysis::m_t destMthd) {
  /*auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IDETypeStateAnalysis::getCallFlowFunction()");*/

  if (destMthd->getName() == "fopen" || destMthd->getName() == "freopen" ||
      destMthd->getName() == "fgetc" || destMthd->getName() == "fputc" ||
      destMthd->getName() == "putchar" || destMthd->getName() == "_IO_getc" ||
      destMthd->getName() == "_I0_putc" || destMthd->getName() == "fprintf" ||
      destMthd->getName() == "__isoc99_fscanf" ||
      destMthd->getName() == "feof" || destMthd->getName() == "ferror" ||
      destMthd->getName() == "ungetc" || destMthd->getName() == "fflush" ||
      destMthd->getName() == "fseek" || destMthd->getName() == "ftell" ||
      destMthd->getName() == "rewind" || destMthd->getName() == "fgetpos" ||
      destMthd->getName() == "fsetpos") {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }

  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destMthd);
  }

  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getRetFlowFunction(IDETypeStateAnalysis::n_t callSite,
                                         IDETypeStateAnalysis::m_t calleeMthd,
                                         IDETypeStateAnalysis::n_t exitStmt,
                                         IDETypeStateAnalysis::n_t retSite) {
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](IDETypeStateAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      });
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::n_t retSite,
    set<IDETypeStateAnalysis::m_t> callees) {
  for (auto Callee : callees) {
    if (Callee->getName() == "fopen") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "freopen") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fgetc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fputc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "putchar") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "_IO_getc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "_I0_putc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fprintf") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "__isoc99_fscanf") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "feof") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "ferror") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "ungetc") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fflush") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fseek") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "ftell") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "rewind") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fgetpos") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }

    if (Callee->getName() == "fsetpos") {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(callSite, zeroValue());
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getSummaryFlowFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::m_t destMthd) {
  return nullptr;
}

map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::initialSeeds() {
  // just start in main()
  map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                             set<IDETypeStateAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IDETypeStateAnalysis::d_t IDETypeStateAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETypeStateAnalysis::isZeroValue(IDETypeStateAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t curr, IDETypeStateAnalysis::d_t currNode,
    IDETypeStateAnalysis::n_t succ, IDETypeStateAnalysis::d_t succNode) {
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          if (currNode == zeroValue() && succNode == Alloca) {
            struct TSEdgeFunction : EdgeFunction<IDETypeStateAnalysis::v_t>,
                                    enable_shared_from_this<TSEdgeFunction> {
              IDETypeStateAnalysis::v_t
              computeTarget(IDETypeStateAnalysis::v_t source) override {
                // TODO adjust the default implementation
                return source;
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
              composeWith(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                              secondFunction) override {
                // TODO adjust the default implementation
                return secondFunction;
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
              joinWith(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                           otherFunction) override {
                // TODO adjust the default implementation
                return otherFunction;
              }
              bool equal_to(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                                other) const override {
                // TODO adjust the default implementation
                return this == other.get();
              }
            };
            // return an instance of the above edge function implementation
            return make_shared<TSEdgeFunction>();
          }
        }
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getCallEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t srcNode,
    IDETypeStateAnalysis::m_t destiantionMethod,
    IDETypeStateAnalysis::d_t destNode) {
  // cout << "Testing this: " << callStmt << " "<< srcNode << " " << endl;
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::m_t calleeMethod,
    IDETypeStateAnalysis::n_t exitStmt, IDETypeStateAnalysis::d_t exitNode,
    IDETypeStateAnalysis::n_t reSite, IDETypeStateAnalysis::d_t retNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getCallToRetEdgeFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode,
    std::set<IDETypeStateAnalysis::m_t> callees) {
  // hier Effekte von open() / close()
  // cout << "callSite: " << callSite->getName().find("open") << " callNode: "
  // << callNode << " retSite: " << retSite->getNumOperands() << "
  // retSiteNode:
  // " << retSiteNode << endl;

  constexpr State delta[4][4] = {
      {State::opened, State::error, State::opened, State::error},
      {State::uninit, State::closed, State::error, State::error},
      {State::error, State::opened, State::error, State::error},
      {State::opened, State::opened, State::opened, State::error},
  };
  // cout << delta[1][2 << endl];
  cout << "callSite: " << callSite << " callNode: " << callNode
       << " retSite: " << retSite << " retSiteNode: " << retSiteNode << endl;
  cout << "Just a test: "
       << EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance() << endl;
  // return
  // delta[static_cast<underlying_type_t<CurrentState>>(curr)][static_cast<underlying_type_t<State>>(state)];

  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode) {
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::v_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::v_t lhs,
                           IDETypeStateAnalysis::v_t rhs) {
  return (lhs == BOTTOM || rhs == BOTTOM) ? BOTTOM : TOP;
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::v_t>>(TOP);
}

// string IDETypeStateAnalysis::VtoString(IDETypeStateAnalysis::v_t v) const {
//   // als erstes implementieren states in strings konvertieren
//   switch (v) {
//     case uninit:
//       return "uninit";
//     case opened:
//       return "opened";
//     case closed:
//       return "closed";
//     case error:
//       return "error";
//     default:
//       return "no state";
//   }
//   return to_string(static_cast<int>(v));
// }

void IDETypeStateAnalysis::printNode(std::ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IDETypeStateAnalysis::printDataFlowFact(std::ostream &os, d_t d) const {
  os << llvmIRToString(d);
}

void IDETypeStateAnalysis::printMethod(ostream &os,
                                       IDETypeStateAnalysis::m_t m) const {
  os << m->getName().str();
}

void IDETypeStateAnalysis::printValue(ostream &os,
                                      IDETypeStateAnalysis::v_t v) const {
  os << to_string(static_cast<int>(v));
}
// NEUES

/*IDETypeStateAnalysis::v_t
IDETypeStateAnalysis::LoadStoreValueIdentity::computeTarget(
    IDETypeStateAnalysis::v_t source) {
  return source;
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::LoadStoreValueIdentity::composeWith(
    shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> secondFunction) {
  return secondFunction;
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::LoadStoreValueIdentity::joinWith(
    shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::v_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::v_t>>(
      IDETypeStateAnalysis::BOTTOM);
}*/

} // namespace psr
