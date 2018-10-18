/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
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

State getNextState(Token tok, State state) {
  return delta[static_cast<std::underlying_type_t<Token>>(tok)]
              [static_cast<std::underlying_type_t<State>>(state)];
}

const State IDETypeStateAnalysis::TOP = State::TOP;
const State IDETypeStateAnalysis::BOTTOM = State::BOT;

const shared_ptr<AllBottom<IDETypeStateAnalysis::v_t>>
    IDETypeStateAnalysis::AllBotFunction(
        make_shared<AllBottom<IDETypeStateAnalysis::v_t>>(
            IDETypeStateAnalysis::BOTTOM));

const std::set<std::string> IDETypeStateAnalysis::STDIOFunctions = {
    "fopen",   "fclose",   "freopen",  "fgetc",   "fputc",
    "putchar", "_IO_getc", "_I0_putc", "fprintf", "__isoc99_fscanf",
    "feof",    "ferror",   "ungetc",   "fflush",  "fseek",
    "ftell",   "rewind",   "fgetpos",  "fsetpos"};

IDETypeStateAnalysis::IDETypeStateAnalysis(IDETypeStateAnalysis::i_t icfg,
                                           vector<string> EntryPoints)
    : DefaultIDETabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIDETabulationProblem::zerovalue = createZeroValue();
}

// start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t curr,
                                            IDETypeStateAnalysis::n_t succ) {
  cout << "NormalFF" << endl;
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
          // we have to generate from value that is loaded!
          struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
            const llvm::LoadInst *Load;
            TSFlowFunction(const llvm::LoadInst *L) : Load(L) {}
            ~TSFlowFunction() override = default;
            set<IDETypeStateAnalysis::d_t>
            computeTargets(IDETypeStateAnalysis::d_t source) override {
              if (source == Load->getPointerOperand()) {
                return {source, Load};
              }
              return {source};
            }
          };
          return make_shared<TSFlowFunction>(Load);
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
          // perform a strong update!
          struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
            const llvm::StoreInst *Store;
            TSFlowFunction(const llvm::StoreInst *S) : Store(S) {}
            ~TSFlowFunction() override = default;
            set<IDETypeStateAnalysis::d_t>
            computeTargets(IDETypeStateAnalysis::d_t source) override {
              if (source == Store->getPointerOperand()) {
                return {};
              }
              if (source == Store->getValueOperand()) {
                return {source, Store->getPointerOperand()};
              }
              return {source};
            }
          };
          return make_shared<TSFlowFunction>(Store);
        }
      }
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t callStmt,
                                          IDETypeStateAnalysis::m_t destMthd) {
  cout << "CallFF" << endl;
  // Kill all data-flow facts if we hit a function that we want to model
  // ourselfs within getCallToRetFlowFunction()
  if (STDIOFunctions.count(destMthd->getName().str())) {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping
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
  cout << "ReturnFF" << endl;
  // Just use the standard mapping
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
  cout << "CallToRetFF" << endl;
  set<std::string> CalleeNames;
  for (auto Callee : callees) {
    CalleeNames.insert(Callee->getName().str());
  }
  llvm::ImmutableCallSite CS(callSite);
  // Generate the return value of fopen()
  if (CS.getCalledFunction()->getName() == "fopen") {
    return make_shared<Gen<IDETypeStateAnalysis::d_t>>(CS.getInstruction(),
                                                       zeroValue());
  }
  // Handle all functions that are not modeld with special semantics
  if (!includes(STDIOFunctions.begin(), STDIOFunctions.end(),
                CalleeNames.begin(), CalleeNames.end())) {
    // Pass all data-flow facts to STDIOFunctions as identity.
    // Kill actual parameters of type '%struct._IO_FILE*' as these
    // data-flow facts are (inter-procedurally) propagated via
    // getCallFlowFunction()
    // and the corresponding getReturnFlowFunction().
    for (auto &Arg : CS.args()) {
      if (Arg->getType()->isPointerTy()) {
        if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
                Arg->getType()->getPointerElementType())) {
          if (StructTy->getName().find("struct._IO_FILE") !=
              llvm::StringRef::npos) {
            return make_shared<Kill<IDETypeStateAnalysis::d_t>>(Arg);
          }
        }
      }
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
  // Set struct._IO_FILE variables to uninitialized once they have been
  // allocated
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find("struct._IO_FILE") !=
            llvm::StringRef::npos) {
          if (currNode == zeroValue() && succNode == Alloca) {
            struct TSEdgeFunction : EdgeFunction<IDETypeStateAnalysis::v_t>,
                                    enable_shared_from_this<TSEdgeFunction> {
              TSEdgeFunction() {}
              ~TSEdgeFunction() override = default;
              IDETypeStateAnalysis::v_t
              computeTarget(IDETypeStateAnalysis::v_t source) override {
                return State::UNINIT;
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
              composeWith(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                              secondFunction) override {
                return make_shared<
                    IDETypeStateAnalysis::TSEdgeFunctionComposer>(
                    this->shared_from_this(), secondFunction);
              }
              shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
              joinWith(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                           otherFunction) override {
                cout << "joinWith(): " << __LINE__ << endl;
                // TODO after discussion
                return this->shared_from_this();
              }
              bool equal_to(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
                                other) const override {
                return this == other.get();
              }
            };
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
  // Set return value from fopen to opened
  llvm::ImmutableCallSite CS(callSite);
  if (CS.getCalledFunction()->getName() == "fopen") {
    if (isZeroValue(callNode) && retSiteNode == CS.getInstruction()) {
      struct TSEdgeFunction : EdgeFunction<IDETypeStateAnalysis::v_t>,
                              enable_shared_from_this<TSEdgeFunction> {
        TSEdgeFunction() {}
        ~TSEdgeFunction() override = default;
        IDETypeStateAnalysis::v_t
        computeTarget(IDETypeStateAnalysis::v_t source) override {
          return State::OPENED;
        }
        shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> composeWith(
            shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> secondFunction)
            override {
          return make_shared<IDETypeStateAnalysis::TSEdgeFunctionComposer>(
              this->shared_from_this(), secondFunction);
        }
        shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> joinWith(
            shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> otherFunction)
            override {
          cout << "joinWith(): " << __LINE__ << endl;
          // TODO after discussion
          return this->shared_from_this();
        }
        bool equal_to(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> other)
            const override {
          return this == other.get();
        }
      };
      return make_shared<TSEdgeFunction>();
    }
  }
  // For all other STDIO functions, that do not generate file handles but only
  // operate on them, model their behavior using a finite state machine.
  if (CS.getCalledFunction()->getName() == "fclose") {
    if (callNode == retSiteNode && callNode == CS.getArgOperand(0)) {
      cout << "fclose processing for: ";
      printDataFlowFact(cout, callNode);
      cout << endl;
      struct TSEdgeFunction : EdgeFunction<IDETypeStateAnalysis::v_t>,
                              enable_shared_from_this<TSEdgeFunction> {
        TSEdgeFunction() { cout << "make edge function for fclose()" << endl; }
        ~TSEdgeFunction() override = default;
        IDETypeStateAnalysis::v_t
        computeTarget(IDETypeStateAnalysis::v_t source) override {
          cout << "computeTarget()" << endl;
          return State::CLOSED;
        }
        shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> composeWith(
            shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> secondFunction)
            override {
          cout << "composeWith(): " << __LINE__ << endl;
          return make_shared<IDETypeStateAnalysis::TSEdgeFunctionComposer>(
              this->shared_from_this(), secondFunction);
        }
        shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> joinWith(
            shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> otherFunction)
            override {
          cout << "joinWith(): " << __LINE__ << endl;
          // TODO after discussion
          return this->shared_from_this();
        }
        bool equal_to(shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> other)
            const override {
          return this == other.get();
        }
      };
      return make_shared<TSEdgeFunction>();
    }
  }
  // Otherwise
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

IDETypeStateAnalysis::v_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::v_t lhs,
                           IDETypeStateAnalysis::v_t rhs) {
  // we use the following lattice
  //                BOT = all information
  //
  // UNINIT   OPENED   CLOSED   ERROR
  //
  //                TOP = no information
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
  case State::TOP:
    os << "TOP";
    break;
  case State::UNINIT:
    os << "UNINIT";
    break;
  case State::OPENED:
    os << "OPENED";
    break;
  case State::CLOSED:
    os << "CLOSED";
    break;
  case State::ERROR:
    os << "ERROR";
    break;
  case State::BOT:
    os << "BOT";
    break;
  default:
    assert(false && "received unknown state!");
    break;
  }
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::TSEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> otherFunction) {
  cout << "IDETypeStateAnalysis::TSEdgeFunctionComposer::joinWith()" << endl;
  // TODO after discussion
  return otherFunction;
}

} // namespace psr

// constexpr State delta[4][4] = {
//     {State::opened, State::error, State::opened, State::error},
//     {State::uninit, State::closed, State::error, State::error},
//     {State::error, State::opened, State::error, State::error},
//     {State::opened, State::opened, State::opened, State::error},
// };

// delta[static_cast<underlying_type_t<CurrentState>>(curr)][static_cast<underlying_type_t<State>>(state)];