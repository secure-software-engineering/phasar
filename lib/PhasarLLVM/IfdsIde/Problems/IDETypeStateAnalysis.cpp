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

/*const std::map<std::string,unsigned> IDETypeStateAnalysis::STDIOFunctions = {
    ("fopen",0),   ("fclose",0),   ("freopen",2),  ("fgetc",0),   ("fputc",1),
   ("putchar",), ("_IO_getc",0), ("_I0_putc",1), ("fprintf",0), ("__isoc99_fscanf",0),
    ("feof",0),    ("ferror",0),   ("ungetc",1),   ("fflush",0),  ("fseek",0),
    ("ftell",0),   ("rewind",0),   ("fgetpos",0),  ("fsetpos",0)};*/

/*const std::set<std::string> IDETypeStateAnalysis::STDIOFunctions = {
    "fopen",   "fclose",   "freopen",  "fgetc",   "fputc",
    "putchar", "_IO_getc", "_I0_putc", "fprintf", "__isoc99_fscanf",
    "feof",    "ferror",   "ungetc",   "fflush",  "fseek",
    "ftell",   "rewind",   "fgetpos",  "fsetpos"};*/ //Standardversion

  /*const std::map<std::string, unsigned> IDETypeStateAnalysis::STDIOFunctions = {
    {"fopen", 0},   {"fclose", 1},   {"freopen", 2},  {"fgetc", 3},   {"fputc", 4},
   {"putchar", 5}, {"_IO_getc", 6}, {"_I0_putc", 7}, {"fprintf", 8}, {"__isoc99_fscanf", 9},
    {"feof", 10},    {"ferror", 11},   {"ungetc", 12},   {"fflush", 13},  {"fseek", 14},
    {"ftell", 15},   {"rewind", 16},   {"fgetpos", 17},  {"fsetpos", 18}};*/

    const std::map<std::string, int> IDETypeStateAnalysis::STDIOFunctions = {
    {"fopen", -1},   {"fclose", 0},   {"freopen", 2},  {"fgetc", 0},   {"fputc", 1},
    {"putchar", -2}, {"_IO_getc", 0}, {"_I0_putc", 1}, {"fprintf",  0}, {"__isoc99_fscanf", 0},
    {"feof", 0},    {"ferror", 0},   {"ungetc", 1},   {"fflush", 0},  {"fseek", 0},
    {"ftell", 0},   {"rewind", 0},   {"fgetpos", 0},  {"fsetpos", 0}};

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
                //Test
                cout << "Testing this source:" << endl;
                source->print(llvm::outs());
                llvm::outs() << '\n';
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
  if (CS.getCalledFunction()->getName() == "fopen" || CS.getCalledFunction()->getName() == "freopen") {
    return make_shared<Gen<IDETypeStateAnalysis::d_t>>(CS.getInstruction(),
                                                       zeroValue());
  }
  // Handle all functions that are not modeld with special semantics
  /*if (!includes(STDIOFunctions.begin(), STDIOFunctions.end(),
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
  }*/
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
            struct TSEdgeFunctionImpl : public TSEdgeFunction {
              TSEdgeFunctionImpl() {}
              IDETypeStateAnalysis::v_t
              computeTarget(IDETypeStateAnalysis::v_t source) override {
                cout << "computeTarget()" << endl;
                //TSEdgeFunctionImpl->value = State::UNINIT;
                //printValue(cout, source);
                return State::UNINIT;
              }
            };
            return make_shared<TSEdgeFunctionImpl>();
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
  if (CS.getCalledFunction()->getName() == "fopen" || CS.getCalledFunction()->getName() == "freopen") {
    if (isZeroValue(callNode) && retSiteNode == CS.getInstruction()) {
      struct TSEdgeFunctionImpl : public TSEdgeFunction {
        TSEdgeFunctionImpl() {}
        IDETypeStateAnalysis::v_t
        computeTarget(IDETypeStateAnalysis::v_t source) override {
          cout << "computeTarget()" << endl;
          //          cout << "Test: " << source << endl;
          //printValue(cout, source);
          ////cout << "Status: " << *retSiteNode->getNextState() << endl;//MY NEW Test
          return State::OPENED; //return delta[0][1]; //return getNextState(Token::FOPEN, State::OPENED);
        }
      };
      return make_shared<TSEdgeFunctionImpl>();
    }
  }
  // For all other STDIO functions, that do not generate file handles but only
  // operate on them, model their behavior using a finite state machine.
  //if (CS.getCalledFunction()->getName() == "fclose") {
  for(auto i : STDIOFunctions){  
    if (CS.getCalledFunction()->getName() == i.first/*"fclose"*/) {
      // Handle parameter itself
      if (callNode == retSiteNode && callNode == CS.getArgOperand(i.second) || (llvm::isa<llvm::LoadInst>(CS.getArgOperand(i.second))) && callNode == llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(i.second))->getPointerOperand()) {
        /**//*if(CS.getArgOperand(i.second) == llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(i.second)))
        {
          cout << "Test:" << endl;
        CS.getArgOperand(0)->print(llvm::outs());
        llvm::outs() << '\n';
          cout << "Test2:" << endl;
        llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(0))->getPointerOperand()->print(llvm::outs());
        llvm::outs() << '\n';
                cout << "Test4:" << endl;
        callSite->print(llvm::outs());
        llvm::outs() << '\n';
        callNode->print(llvm::outs());
        llvm::outs() << '\n';       
        retSite->print(llvm::outs());
        llvm::outs() << '\n';
        retSiteNode->print(llvm::outs());
        llvm::outs() << '\n';
        /*callees->print(llvm::outs());
        llvm::outs() << '\n';*/
        // Hier muss noch die übergabe des close an allocaanweisung geschehen über computeTarget den source von llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(0))->getPointerOperand() auf CLOSED


        /*for(auto Callee : callees){

          cout << "Callee:" << endl;
          Callee->print(llvm::outs());
          llvm::outs() << '\n';
        }

        //llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(0))->getPointerOperand()->computeTargets(State::CLOSED);
        }/**/
        if(i.first == "fclose"){
        cout << " fclose processing for: ";
        printDataFlowFact(cout, callNode);
        cout << endl;
        //test
        /*if(CS.getArgOperand(i.second) == llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(i.second)))
        {
          // Hier muss die Funktion das llvm::dyn_cast<llvm::LoadInst>(CS.getArgOperand(0))->getPointerOperand() auf CLOSE über computeTarget setzen
        }*/
        struct TSEdgeFunctionImpl : public TSEdgeFunction {
          TSEdgeFunctionImpl() {
            cout << "make edge function for fclose()" << endl;
          }
          IDETypeStateAnalysis::v_t
          computeTarget(IDETypeStateAnalysis::v_t source) override {
            cout << "computeTarget()" << endl;
            //if(i.first == "fclose"){
            return getNextState(Token::FCLOSE, source);//}
            // TODO insert automaton as fclose() is a consuming function
            //return State::CLOSED;
          }
        };
        return make_shared<TSEdgeFunctionImpl>();
        }
        // Test vermutlich eleganter lösbar
        else {
        cout << " star processing for: ";
        printDataFlowFact(cout, callNode);
        cout << endl;
        struct TSEdgeFunctionImpl : public TSEdgeFunction {
          TSEdgeFunctionImpl() {
            cout << "make edge function for starfunction()" << endl;
          }
          IDETypeStateAnalysis::v_t
          computeTarget(IDETypeStateAnalysis::v_t source) override {
            cout << "computeTarget()" << endl;
            //if(i.first == "fclose"){
            return getNextState(Token::STAR, source);//}
            // TODO insert automaton as fclose() is a consuming function
          }
        };
        return make_shared<TSEdgeFunctionImpl>();
        }
        // Test end
      }
      // TODO handle allocated file handle (i) follow use-def chain, (ii) give it
      // the
      // same state as the parameter of the consuming function
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
  } else if (lhs == rhs) {
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
  if (dynamic_cast<AllTop<IDETypeStateAnalysis::v_t> *>(this) &&
      !dynamic_cast<AllBottom<IDETypeStateAnalysis::v_t> *>(
          otherFunction.get())) {
    return otherFunction;
  }
  if (dynamic_cast<AllTop<IDETypeStateAnalysis::v_t> *>(otherFunction.get()) &&
      !dynamic_cast<AllBottom<IDETypeStateAnalysis::v_t> *>(this)) {
    return this->shared_from_this();
  } else if (this->equal_to(otherFunction)) {
    return this->shared_from_this();
  } else {
    return AllBotFunction;
  }
  return otherFunction;
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::TSEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> secondFunction) {
  return make_shared<TSEdgeFunctionComposer>(this->shared_from_this(),
                                             secondFunction);
}

// this implementation must of course be consistent with the implementation
// of the JoinLattice
std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>>
IDETypeStateAnalysis::TSEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> otherFunction) {
  // compare with the (one-level) lattice used by this analysis
  if (dynamic_cast<AllTop<IDETypeStateAnalysis::v_t> *>(this) &&
      !dynamic_cast<AllBottom<IDETypeStateAnalysis::v_t> *>(
          otherFunction.get())) {
    return otherFunction;
  }
  if (dynamic_cast<AllTop<IDETypeStateAnalysis::v_t> *>(otherFunction.get()) &&
      !dynamic_cast<AllBottom<IDETypeStateAnalysis::v_t> *>(this)) {
    return this->shared_from_this();
  } else if (this->equal_to(otherFunction)) {
    return this->shared_from_this();
  } else {
    return AllBotFunction;
  }
}

bool IDETypeStateAnalysis::TSEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::v_t>> other) const {
  // TODO needs improvement
  return this == other.get();
}

} // namespace psr
