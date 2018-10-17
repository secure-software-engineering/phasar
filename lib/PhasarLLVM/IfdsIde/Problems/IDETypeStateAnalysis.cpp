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
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>
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

const std::set<std::string> IDETypeStateAnalysis::STDIOFunctions = {
    "fopen",   "fclose",   "freopen",  "fgetc",   "fputc",
    "putchar", "_IO_getc", "_I0_putc", "fprintf", "__isoc99_fscanf",
    "feof",    "ferror",   "ungetc",   "fflush",  "fseek",
    "ftell",   "rewind",   "fgetpos",  "fsetpos"};

const std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
    IDETypeStateAnalysis::AllBot(
        make_shared<AllBottom<IDETypeStateAnalysis::v_t>>(BOTTOM));

IDETypeStateAnalysis::IDETypeStateAnalysis(IDETypeStateAnalysis::i_t icfg,
                                           vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t curr,
                                            IDETypeStateAnalysis::n_t succ) {
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
  }
  // check load instructions for file handler
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load->getPointerOperand()
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
    }
  }
  // check store instructions for file handler
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Store->getValueOperand()->getType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          return make_shared<Gen<IDETypeStateAnalysis::d_t>>(
              Store->getPointerOperand(), zeroValue());
        }
      }
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t callStmt,
                                          IDETypeStateAnalysis::m_t destMthd) {
  if (STDIOFunctions.count(destMthd->getName().str())) {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destMthd);
  }
  assert(false && "callStmt not a CallInst nor a InvokeInst");
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
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::n_t retSite,
    set<IDETypeStateAnalysis::m_t> callees) {
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
  // if (currNode == zeroValue() && succNode == zeroValue()) {
  //   return AllBot;
  // }
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          if (currNode == zeroValue() && succNode == Alloca) {
            cout << "FOUND CASE!" << endl;
            struct TSEdgeFunction : EdgeFunction<IDETypeStateAnalysis::v_t>,
                                    enable_shared_from_this<TSEdgeFunction> {
              IDETypeStateAnalysis::v_t computeTarget(
                  IDETypeStateAnalysis::v_t source) override {
                cout << "computeTarget(" << source << ") -> " << uninit << endl;
                return uninit;
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> composeWith(
                  shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                      secondFunction) override {
                return this->shared_from_this();
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> joinWith(
                  shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                      otherFunction) override {
                return this->shared_from_this();
              }
              bool equal_to(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                                other) const override {
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
  return EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode) {
  return nullptr;
}

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::v_t IDETypeStateAnalysis::join(
    IDETypeStateAnalysis::v_t lhs, IDETypeStateAnalysis::v_t rhs) {
  if (lhs == TOP && rhs != BOTTOM) {
    return rhs;
  } else if (rhs == TOP && lhs != BOTTOM) {
    return lhs;
  } else {
    return BOTTOM;
  }
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::v_t>>(TOP);
}

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
  switch (v) {
    case uninit:
      os << "uninit";
      break;
    case opened:
      os << "opened";
      break;
    case closed:
      os << "closed";
      break;
    case error:
      os << "error";
      break;
    default:
      assert(false && "received unknown state!");
      break;
  }
}

}  // namespace psr

// // hier Effekte von open() / close()
// // cout << "callSite: " << callSite->getName().find("open") << " callNode:
// "
// // << callNode << " retSite: " << retSite->getNumOperands() << "
// // retSiteNode:
// // " << retSiteNode << endl;

// constexpr State delta[4][4] = {
//     {State::opened, State::error, State::opened, State::error},
//     {State::uninit, State::closed, State::error, State::error},
//     {State::error, State::opened, State::error, State::error},
//     {State::opened, State::opened, State::opened, State::error},
// };
// // cout << delta[1][2 << endl];
// cout << "callSite: " << callSite << " callNode: " << callNode
//      << " retSite: " << retSite << " retSiteNode: " << retSiteNode << endl;
// cout << "Just a test: "
//      << EdgeIdentity<IDETypeStateAnalysis::v_t>::getInstance() << endl;
// // return
// //
// delta[static_cast<underlying_type_t<CurrentState>>(curr)][static_cast<underlying_type_t<State>>(state)];