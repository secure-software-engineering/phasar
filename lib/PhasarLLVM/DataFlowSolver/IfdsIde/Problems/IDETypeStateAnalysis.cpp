/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Kill.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillMultiple.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/LambdaFlow.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/PropagateLoad.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

IDETypeStateAnalysis::IDETypeStateAnalysis(const ProjectIRDB *IRDB,
                                           const LLVMTypeHierarchy *TH,
                                           const LLVMBasedICFG *ICF,
                                           const LLVMPointsToInfo *PT,
                                           const TypeStateDescription &TSD,
                                           std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints), TSD(TSD),
      TOP(TSD.top()), BOTTOM(TSD.bottom()) {
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// Start formulating our analysis by specifying the parts required for IFDS

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t curr,
                                            IDETypeStateAnalysis::n_t succ) {
  // Check if Alloca's type matches the target type. If so, generate from zero
  // value.
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (hasMatchingType(Alloca)) {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Alloca,
                                                         getZeroValue());
    }
  }
  // Check load instructions for target type. Generate from the loaded value and
  // kill the load instruction if it was generated previously (strong update!).
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (hasMatchingType(Load)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        const llvm::LoadInst *Load;

        TSFlowFunction(const llvm::LoadInst *L) : Load(L) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t source) override {
          if (source == Load) {
            return {};
          }
          if (source == Load->getPointerOperand()) {
            return {source, Load};
          }
          return {source};
        }
      };
      return make_shared<TSFlowFunction>(Load);
    }
  }
  if (auto Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(curr)) {
    if (hasMatchingType(Gep->getPointerOperand())) {
      return make_shared<LambdaFlow<d_t>>([=](d_t source) -> set<d_t> {
        // if (source == Gep->getPointerOperand()) {
        //  return {source, Gep};
        //}
        return {source};
      });
    }
  }
  // Check store instructions for target type. Perform a strong update, i.e.
  // kill the alloca pointed to by the pointer-operand and all alloca's related
  // to the value-operand and then generate them from the value-operand.
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    if (hasMatchingType(Store)) {
      auto RelevantAliasesAndAllocas = getLocalAliasesAndAllocas(
          Store->getPointerOperand(), // pointer- or value operand???
          // Store->getValueOperand(),
          curr->getParent()->getParent()->getName().str());

      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        const llvm::StoreInst *Store;
        std::set<IDETypeStateAnalysis::d_t> AliasesAndAllocas;
        TSFlowFunction(const llvm::StoreInst *S,
                       std::set<IDETypeStateAnalysis::d_t> AA)
            : Store(S), AliasesAndAllocas(AA) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t source) override {
          // We kill all relevant loacal aliases and alloca's
          if (source != Store->getValueOperand() &&
              // AliasesAndAllocas.find(source) != AliasesAndAllocas.end()
              // Is simple comparison sufficient?
              source == Store->getPointerOperand()) {
            return {};
          }
          // Generate all local aliases and relevant alloca's from the stored
          // value
          if (source == Store->getValueOperand()) {
            AliasesAndAllocas.insert(source);
            return AliasesAndAllocas;
          }
          return {source};
        }
      };
      return make_shared<TSFlowFunction>(Store, RelevantAliasesAndAllocas);
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t callStmt,
                                          IDETypeStateAnalysis::f_t destFun) {
  // Kill all data-flow facts if we hit a function of the target API.
  // Those functions are modled within Call-To-Return.
  if (TSD.isAPIFunction(cxx_demangle(destFun->getName().str()))) {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping.
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destFun);
  }
  assert(false && "callStmt not a CallInst nor a InvokeInst");
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getRetFlowFunction(IDETypeStateAnalysis::n_t callSite,
                                         IDETypeStateAnalysis::f_t calleeFun,
                                         IDETypeStateAnalysis::n_t exitStmt,
                                         IDETypeStateAnalysis::n_t retSite) {
  // Besides mapping the formal parameter back into the actual parameter and
  // propagating the return value into the caller context, we also propagate
  // all related alloca's of the formal parameter and the return value.
  struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
    llvm::ImmutableCallSite CallSite;
    const llvm::Function *calleeFun;
    const llvm::ReturnInst *ExitStmt;
    IDETypeStateAnalysis *Analysis;
    std::vector<const llvm::Value *> actuals;
    std::vector<const llvm::Value *> formals;
    TSFlowFunction(llvm::ImmutableCallSite cs, const llvm::Function *calleeFun,
                   const llvm::Instruction *exitstmt,
                   IDETypeStateAnalysis *analysis)
        : CallSite(cs), calleeFun(calleeFun),
          ExitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)),
          Analysis(analysis) {
      // Set up the actual parameters
      for (unsigned idx = 0; idx < CallSite.getNumArgOperands(); ++idx) {
        actuals.push_back(CallSite.getArgOperand(idx));
      }
      // Set up the formal parameters
      for (unsigned idx = 0; idx < calleeFun->arg_size(); ++idx) {
        formals.push_back(getNthFunctionArgument(calleeFun, idx));
      }
    }

    ~TSFlowFunction() override = default;

    set<IDETypeStateAnalysis::d_t>
    computeTargets(IDETypeStateAnalysis::d_t source) override {
      if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(source)) {
        set<const llvm::Value *> res;
        // Handle C-style varargs functions
        if (calleeFun->isVarArg() && !calleeFun->isDeclaration()) {
          const llvm::Instruction *AllocVarArg;
          // Find the allocation of %struct.__va_list_tag
          for (auto &BB : *calleeFun) {
            for (auto &I : BB) {
              if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                if (Alloc->getAllocatedType()->isArrayTy() &&
                    Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                    Alloc->getAllocatedType()
                        ->getArrayElementType()
                        ->isStructTy() &&
                    Alloc->getAllocatedType()
                            ->getArrayElementType()
                            ->getStructName() == "struct.__va_list_tag") {
                  AllocVarArg = Alloc;
                  // TODO break out this nested loop earlier (without goto ;-)
                }
              }
            }
          }
          // Generate the varargs things by using an over-approximation
          if (source == AllocVarArg) {
            for (unsigned idx = formals.size(); idx < actuals.size(); ++idx) {
              res.insert(actuals[idx]);
            }
          }
        }
        // Handle ordinary case
        // Map formal parameter into corresponding actual parameter.
        for (unsigned idx = 0; idx < formals.size(); ++idx) {
          if (source == formals[idx]) {
            res.insert(actuals[idx]); // corresponding actual
          }
        }
        // Collect the return value
        if (source == ExitStmt->getReturnValue()) {
          res.insert(CallSite.getInstruction());
        }
        // Collect all relevant alloca's to map into caller context
        std::set<IDETypeStateAnalysis::d_t> RelAllocas;
        for (auto fact : res) {
          auto allocas = Analysis->getRelevantAllocas(fact);
          RelAllocas.insert(allocas.begin(), allocas.end());
        }
        res.insert(RelAllocas.begin(), RelAllocas.end());
        return res;
      } else {
        return {source};
      }
    }
  };
  return make_shared<TSFlowFunction>(llvm::ImmutableCallSite(callSite),
                                     calleeFun, exitStmt, this);
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::n_t retSite,
    set<IDETypeStateAnalysis::f_t> callees) {
  const llvm::ImmutableCallSite CS(callSite);
  for (auto Callee : callees) {
    std::string demangledFname = cxx_demangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (TSD.isFactoryFunction(demangledFname)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        IDETypeStateAnalysis::d_t CallSite, ZeroValue;

        TSFlowFunction(IDETypeStateAnalysis::d_t CS,
                       IDETypeStateAnalysis::d_t Z)
            : CallSite(CS), ZeroValue(Z) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t source) override {
          if (source == CallSite) {
            return {};
          }
          if (source == ZeroValue) {
            return {source, CallSite};
          }
          return {source};
        }
      };
      return make_shared<TSFlowFunction>(callSite, getZeroValue());
    }

    // Handle all functions that are not modeld with special semantics.
    // Kill actual parameters of target type and all its aliases
    // and the corresponding alloca(s) as these data-flow facts are
    // (inter-procedurally) propagated via Call- and the corresponding
    // Return-Flow. Otherwise we might propagate facts with not updated
    // states.
    // Alloca's related to the return value of non-api functions will
    // not be killed during call-to-return, since it is not safe to assume
    // that the return value will be used afterwards, i.e. is stored to memory
    // pointed to by related alloca's.
    if (!TSD.isAPIFunction(demangledFname) && !Callee->isDeclaration()) {
      for (auto &Arg : CS.args()) {
        if (hasMatchingType(Arg)) {
          std::set<IDETypeStateAnalysis::d_t> FactsToKill =
              getWMAliasesAndAllocas(Arg.get());
          return make_shared<KillMultiple<IDETypeStateAnalysis::d_t>>(
              FactsToKill);
        }
      }
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::getSummaryFlowFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::f_t destFun) {
  return nullptr;
}

map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::initialSeeds() {
  // just start in main()
  map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IDETypeStateAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDETypeStateAnalysis::d_t IDETypeStateAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETypeStateAnalysis::isZeroValue(IDETypeStateAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t curr, IDETypeStateAnalysis::d_t currNode,
    IDETypeStateAnalysis::n_t succ, IDETypeStateAnalysis::d_t succNode) {
  // Set alloca instructions of target type to uninitialized.
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (hasMatchingType(Alloca)) {
      if (currNode == getZeroValue() && succNode == Alloca) {
        struct TSAllocaEF : public EdgeFunction<l_t>,
                            public std::enable_shared_from_this<TSAllocaEF> {
          const TypeStateDescription &TSD;
          l_t CurrentState;
          const llvm::AllocaInst *Alloca;
          TSAllocaEF(const TypeStateDescription &tsd,
                     const llvm::AllocaInst *Alloca)
              : TSD(tsd), CurrentState(tsd.top()), Alloca(Alloca) {}

          IDETypeStateAnalysis::l_t
          computeTarget(IDETypeStateAnalysis::l_t source) override {
            // std::cerr << "UNINIT INITIALIZATION: " << llvmIRToString(Alloca)
            //          << std::endl;
            CurrentState = TSD.uninit();
            return CurrentState;
          }

          void print(std::ostream &OS, bool isForDebug = false) const override {
            OS << "Alloca(" << TSD.stateToString(CurrentState) << ")";
          }

          bool equal_to(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                            other) const override {
            if (auto *TSEF =
                    dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(
                        other.get())) {
              return this->CurrentState == TSEF->getCurrentState();
            }
            return this == other.get();
          }
          std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
          composeWith(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                          secondFunction) override {
            if (auto *AB = dynamic_cast<AllBottom<IDETypeStateAnalysis::l_t> *>(
                    secondFunction.get())) {
              return this->shared_from_this();
            }
            if (auto *EI =
                    dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
                        secondFunction.get())) {
              return this->shared_from_this();
            }
            return make_shared<TSEdgeFunctionComposer>(
                this->shared_from_this(), secondFunction, TSD.bottom());
          }

          std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
          joinWith(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                       otherFunction) override {
            if (otherFunction.get() == this ||
                otherFunction->equal_to(this->shared_from_this())) {
              return this->shared_from_this();
            }
            if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
                    otherFunction.get())) {
              return this->shared_from_this();
            }
            return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(
                TSD.bottom());
          }
        };
        return make_shared<TSAllocaEF>(TSD, Alloca);
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getCallEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t srcNode,
    IDETypeStateAnalysis::f_t destinationFunction,
    IDETypeStateAnalysis::d_t destNode) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t callSite,
    IDETypeStateAnalysis::f_t calleeFunction,
    IDETypeStateAnalysis::n_t exitStmt, IDETypeStateAnalysis::d_t exitNode,
    IDETypeStateAnalysis::n_t reSite, IDETypeStateAnalysis::d_t retNode) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getCallToRetEdgeFunction(
    IDETypeStateAnalysis::n_t callSite, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode,
    std::set<IDETypeStateAnalysis::f_t> callees) {
  auto &lg = lg::get();
  const llvm::ImmutableCallSite CS(callSite);
  for (auto Callee : callees) {
    std::string demangledFname = cxx_demangle(Callee->getName().str());

    // For now we assume that we can only generate from the return value.
    // We apply the same edge function for the return value, i.e. callsite.
    if (TSD.isFactoryFunction(demangledFname)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Processing factory function");
      if (isZeroValue(callNode) && retSiteNode == CS.getInstruction()) {
        struct TSFactoryEF : public TSEdgeFunction {
          TSFactoryEF(const TypeStateDescription &tsd, const std::string &tok,
                      llvm::ImmutableCallSite cs)
              : TSEdgeFunction(tsd, tok, cs) {}

          IDETypeStateAnalysis::l_t
          computeTarget(IDETypeStateAnalysis::l_t source) override {
            // CurrentState = TSD.start();
            CurrentState = TSD.getNextState(
                Token, source == TSD.top() ? TSD.uninit() : source, CS);
            return CurrentState;
          }

          void print(std::ostream &OS, bool isForDebug = false) const override {
            OS << "Factory(" << TSD.stateToString(CurrentState) << ")";
          }
        };
        return make_shared<TSFactoryEF>(TSD, demangledFname, CS);
      }
    }

    // For every consuming parameter and all its aliases and relevant alloca's
    // we apply the same edge function.
    if (TSD.isConsumingFunction(demangledFname)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Processing consuming function");
      for (auto Idx : TSD.getConsumerParamIdx(demangledFname)) {
        std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas =
            getWMAliasesAndAllocas(CS.getArgument(Idx));

        if (callNode == retSiteNode &&
            PointsToAndAllocas.find(callNode) != PointsToAndAllocas.end()) {
          return make_shared<TSEdgeFunction>(TSD, demangledFname, CS);
        }
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t callStmt, IDETypeStateAnalysis::d_t callNode,
    IDETypeStateAnalysis::n_t retSite, IDETypeStateAnalysis::d_t retSiteNode) {
  return nullptr;
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::l_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::l_t lhs,
                           IDETypeStateAnalysis::l_t rhs) {
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

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::l_t>>(TOP);
}

void IDETypeStateAnalysis::printNode(std::ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IDETypeStateAnalysis::printDataFlowFact(std::ostream &os, d_t d) const {
  os << llvmIRToString(d);
}

void IDETypeStateAnalysis::printFunction(ostream &os,
                                         IDETypeStateAnalysis::f_t m) const {
  os << m->getName().str();
}

void IDETypeStateAnalysis::printEdgeFact(ostream &os,
                                         IDETypeStateAnalysis::l_t l) const {
  os << TSD.stateToString(l);
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(botElement);
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::TSEdgeFunction::computeTarget(
    IDETypeStateAnalysis::l_t source) {
  auto &lg = lg::get();
  CurrentState = TSD.getNextState(Token, source);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "State machine transition: (" << Token << " , "
                << TSD.stateToString(source) << ") -> "
                << TSD.stateToString(CurrentState));
  return CurrentState;
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> secondFunction) {
  if (auto *AB = dynamic_cast<AllBottom<IDETypeStateAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<TSEdgeFunctionComposer>(this->shared_from_this(),
                                             secondFunction, TSD.bottom());
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  // if (auto *EI = dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
  //         otherFunction.get())) {
  //   return this->shared_from_this();
  // }
  if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(TSD.bottom());
}

bool IDETypeStateAnalysis::TSEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> other) const {
  if (auto *TSEF =
          dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(other.get())) {
    return this->CurrentState == TSEF->CurrentState;
  }
  return this == other.get();
}

void IDETypeStateAnalysis::TSEdgeFunction::print(ostream &OS,
                                                 bool isForDebug) const {
  OS << "TSEF(" << TSD.stateToString(CurrentState) << ")";
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getRelevantAllocas(IDETypeStateAnalysis::d_t V) {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  } else {
    auto PointsToSet = getWMPointsToSet(V);
    std::set<IDETypeStateAnalysis::d_t> RelevantAllocas;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Compute relevant alloca's of "
                  << IDETypeStateAnalysis::DtoString(V));
    for (auto Alias : PointsToSet) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Alias: " << IDETypeStateAnalysis::DtoString(Alias));
      // Collect the pointer operand of a aliased load instruciton
      if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
        if (hasMatchingType(Alias)) {
          LOG_IF_ENABLE(
              BOOST_LOG_SEV(lg, DEBUG)
              << " -> Alloca: "
              << IDETypeStateAnalysis::DtoString(Load->getPointerOperand()));
          RelevantAllocas.insert(Load->getPointerOperand());
        }
      } else {
        // For all other types of aliases, e.g. callsites, function arguments,
        // we check store instructions where thoses aliases are value operands.
        for (auto User : Alias->users()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "  User: " << IDETypeStateAnalysis::DtoString(User));
          if (auto Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
            if (hasMatchingType(Store)) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                            << "    -> Alloca: "
                            << IDETypeStateAnalysis::DtoString(
                                   Store->getPointerOperand()));
              RelevantAllocas.insert(Store->getPointerOperand());
            }
          }
        }
      }
    }
    for (auto Alias : PointsToSet) {
      RelevantAllocaCache[Alias] = RelevantAllocas;
    }
    return RelevantAllocas;
  }
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMPointsToSet(IDETypeStateAnalysis::d_t V) {
  if (PointsToCache.find(V) != PointsToCache.end()) {
    return PointsToCache[V];
  } else {
    auto PointsToSet = ICF->getWholeModulePTG().getPointsToSet(V);
    for (auto Alias : PointsToSet) {
      if (hasMatchingType(Alias))
        PointsToCache[Alias] = PointsToSet;
    }
    return PointsToSet;
  }
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMAliasesAndAllocas(IDETypeStateAnalysis::d_t V) {
  std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas;
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas = getRelevantAllocas(V);
  std::set<IDETypeStateAnalysis::d_t> Aliases = getWMPointsToSet(V);
  PointsToAndAllocas.insert(Aliases.begin(), Aliases.end());
  PointsToAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return PointsToAndAllocas;
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getLocalAliasesAndAllocas(IDETypeStateAnalysis::d_t V,
                                                const std::string &Fname) {
  std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas;
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas = getRelevantAllocas(V);
  std::set<IDETypeStateAnalysis::d_t>
      Aliases; // =
               // IRDB->getPointsToGraph(Fname)->getPointsToSet(V);
  for (auto Alias : Aliases) {
    if (hasMatchingType(Alias))
      PointsToAndAllocas.insert(Alias);
  }
  // PointsToAndAllocas.insert(Aliases.begin(), Aliases.end());
  PointsToAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return PointsToAndAllocas;
}
bool hasMatchingTypeName(const llvm::Type *ty, const std::string &pattern) {
  if (auto StructTy = llvm::dyn_cast<llvm::StructType>(ty)) {
    return StructTy->getName().contains(pattern);
  } else {
    // primitive type
    std::string str;
    llvm::raw_string_ostream s(str);
    s << *ty;
    s.flush();
    return str.find(pattern) != std::string::npos;
  }
}
bool IDETypeStateAnalysis::hasMatchingType(IDETypeStateAnalysis::d_t V) {
  // General case
  if (V->getType()->isPointerTy()) {
    if (hasMatchingTypeName(V->getType()->getPointerElementType(),
                            TSD.getTypeNameOfInterest()))
      return true;
  }
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Alloca->getAllocatedType()->getPointerElementType(),
              TSD.getTypeNameOfInterest()))
        return true;
    }
    return false;
  }
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getPointerOperand()
            ->getType()
            ->getPointerElementType()
            ->isPointerTy()) {
      if (hasMatchingTypeName(Load->getPointerOperand()
                                  ->getType()
                                  ->getPointerElementType()
                                  ->getPointerElementType(),
                              TSD.getTypeNameOfInterest()))
        return true;
    }
    return false;
  }
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Store->getValueOperand()->getType()->getPointerElementType(),
              TSD.getTypeNameOfInterest()))
        return true;
    }
    return false;
  }
  return false;
}

void IDETypeStateAnalysis::emitTextReport(
    const SolverResults<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                        IDETypeStateAnalysis::l_t> &SR,
    std::ostream &os) {
  os << "\n======= TYPE STATE RESULTS =======\n";
  for (auto &f : ICF->getAllFunctions()) {
    os << '\n' << getFunctionNameFromIR(f) << '\n';
    for (auto &BB : *f) {
      for (auto &I : BB) {
        auto results = SR.resultsAt(&I, true);
        if (ICF->isExitStmt(&I)) {
          os << "\nAt exit stmt: " << NtoString(&I) << '\n';
          for (auto res : results) {
            if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(res.first)) {
              if (res.second == TSD.error()) {
                os << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(res.first) << '\n';
                for (auto Pred : ICF->getPredsOf(&I)) {
                  os << "\nPredecessor: " << NtoString(Pred) << '\n';
                  auto PredResults = SR.resultsAt(Pred, true);
                  for (auto Res : PredResults) {
                    if (Res.first == Alloca) {
                      os << "Pred State: " << LtoString(Res.second) << '\n';
                    }
                  }
                }
                os << "============================\n";
              } else {
                os << "\nAlloca : " << DtoString(res.first)
                   << "\nState  : " << LtoString(res.second) << '\n';
              }
            } else {
              os << "\nInst: " << NtoString(&I) << endl
                 << "Fact: " << DtoString(res.first) << endl
                 << "State: " << LtoString(res.second) << endl;
            }
          }
        } else {
          for (auto res : results) {
            if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(res.first)) {
              if (res.second == TSD.error()) {
                os << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(res.first) << '\n'
                   << "\nAt IR Inst: " << NtoString(&I) << '\n';
                for (auto Pred : ICF->getPredsOf(&I)) {
                  os << "\nPredecessor: " << NtoString(Pred) << '\n';
                  auto PredResults = SR.resultsAt(Pred, true);
                  for (auto Res : PredResults) {
                    if (Res.first == Alloca) {
                      os << "Pred State: " << LtoString(Res.second) << '\n';
                    }
                  }
                }
                os << "============================\n";
              }
            } else {
              os << "\nInst: " << NtoString(&I) << endl
                 << "Fact: " << DtoString(res.first) << endl
                 << "State: " << LtoString(res.second) << endl;
            }
          }
        }
      }
    }
    os << "\n--------------------------------------------\n";
  }
}

} // namespace psr
