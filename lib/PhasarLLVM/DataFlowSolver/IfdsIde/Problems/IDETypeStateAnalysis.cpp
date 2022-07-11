/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <utility>

#include "llvm/ADT/DenseMap.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
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
                                           LLVMPointsToInfo *PT,
                                           const TypeStateDescription &TSD,
                                           std::set<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)), TSD(TSD),
      TOP(TSD.top()), BOTTOM(TSD.bottom()) {
  IDETabulationProblem::ZeroValue = IDETypeStateAnalysis::createZeroValue();
}

// Start formulating our analysis by specifying the parts required for IFDS

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getNormalFlowFunction(
    IDETypeStateAnalysis::n_t Curr, IDETypeStateAnalysis::n_t /*Succ*/) {
  // Check if Alloca's type matches the target type. If so, generate from zero
  // value.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      return make_shared<Gen<IDETypeStateAnalysis::d_t>>(Alloca,
                                                         getZeroValue());
    }
  }
  // Check load instructions for target type. Generate from the loaded value and
  // kill the load instruction if it was generated previously (strong update!).
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    if (hasMatchingType(Load)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        const llvm::LoadInst *Load;

        TSFlowFunction(const llvm::LoadInst *L) : Load(L) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t Source) override {
          if (Source == Load) {
            return {};
          }
          if (Source == Load->getPointerOperand()) {
            return {Source, Load};
          }
          return {Source};
        }
      };
      return make_shared<TSFlowFunction>(Load);
    }
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    if (hasMatchingType(Gep->getPointerOperand())) {
      return makeLambdaFlow<d_t>([=](d_t Source) -> set<d_t> {
        // if (Source == Gep->getPointerOperand()) {
        //  return {Source, Gep};
        //}
        return {Source};
      });
    }
  }
  // Check store instructions for target type. Perform a strong update, i.e.
  // kill the alloca pointed to by the pointer-operand and all alloca's related
  // to the value-operand and then generate them from the value-operand.
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    if (hasMatchingType(Store)) {
      auto RelevantAliasesAndAllocas = getLocalAliasesAndAllocas(
          Store->getPointerOperand(), // pointer- or value operand???
          // Store->getValueOperand(),
          Curr->getParent()->getParent()->getName().str());

      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        const llvm::StoreInst *Store;
        std::set<IDETypeStateAnalysis::d_t> AliasesAndAllocas;
        TSFlowFunction(const llvm::StoreInst *S,
                       std::set<IDETypeStateAnalysis::d_t> AA)
            : Store(S), AliasesAndAllocas(std::move(AA)) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t Source) override {
          // We kill all relevant loacal aliases and alloca's
          if (Source != Store->getValueOperand() &&
              // AliasesAndAllocas.find(Source) != AliasesAndAllocas.end()
              // Is simple comparison sufficient?
              Source == Store->getPointerOperand()) {
            return {};
          }
          // Generate all local aliases and relevant alloca's from the stored
          // value
          if (Source == Store->getValueOperand()) {
            AliasesAndAllocas.insert(Source);
            return AliasesAndAllocas;
          }
          return {Source};
        }
      };
      return make_shared<TSFlowFunction>(Store, RelevantAliasesAndAllocas);
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t CallSite,
                                          IDETypeStateAnalysis::f_t DestFun) {
  // Kill all data-flow facts if we hit a function of the target API.
  // Those functions are modled within Call-To-Return.
  if (TSD.isAPIFunction(llvm::demangle(DestFun->getName().str()))) {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping.
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    return make_shared<MapFactsToCallee<>>(llvm::cast<llvm::CallBase>(CallSite),
                                           DestFun);
  }
  llvm::report_fatal_error("callSite not a CallInst nor a InvokeInst");
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getRetFlowFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::f_t CalleeFun,
    IDETypeStateAnalysis::n_t ExitStmt, IDETypeStateAnalysis::n_t /*RetSite*/) {
  // Besides mapping the formal parameter back into the actual parameter and
  // propagating the return value into the caller context, we also propagate
  // all related alloca's of the formal parameter and the return value.
  struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
    const llvm::CallBase *CallSite;
    const llvm::Function *CalleeFun;
    const llvm::ReturnInst *ExitSite;
    IDETypeStateAnalysis *Analysis;
    std::vector<const llvm::Value *> Actuals;
    std::vector<const llvm::Value *> Formals;
    TSFlowFunction(const llvm::CallBase *CallSite,
                   const llvm::Function *CalleeFun,
                   const llvm::Instruction *ExitSite,
                   IDETypeStateAnalysis *Analysis)
        : CallSite(CallSite), CalleeFun(CalleeFun),
          ExitSite(llvm::dyn_cast<llvm::ReturnInst>(ExitSite)),
          Analysis(Analysis) {
      // Set up the actual parameters
      for (unsigned Idx = 0; Idx < CallSite->arg_size(); ++Idx) {
        Actuals.push_back(CallSite->getArgOperand(Idx));
      }
      // Set up the formal parameters
      for (unsigned Idx = 0; Idx < CalleeFun->arg_size(); ++Idx) {
        Formals.push_back(getNthFunctionArgument(CalleeFun, Idx));
      }
    }

    ~TSFlowFunction() override = default;

    set<IDETypeStateAnalysis::d_t>
    computeTargets(IDETypeStateAnalysis::d_t Source) override {
      if (!LLVMZeroValue::isLLVMZeroValue(Source)) {
        set<const llvm::Value *> Res;
        // Handle C-style varargs functions
        if (CalleeFun->isVarArg() && !CalleeFun->isDeclaration()) {
          const llvm::Instruction *AllocVarArg;
          // Find the allocation of %struct.__va_list_tag
          for (const auto &BB : *CalleeFun) {
            for (const auto &I : BB) {
              if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
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
          if (Source == AllocVarArg) {
            for (unsigned Idx = Formals.size(); Idx < Actuals.size(); ++Idx) {
              Res.insert(Actuals[Idx]);
            }
          }
        }
        // Handle ordinary case
        // Map formal parameter into corresponding actual parameter.
        for (unsigned Idx = 0; Idx < Formals.size(); ++Idx) {
          if (Source == Formals[Idx]) {
            Res.insert(Actuals[Idx]); // corresponding actual
          }
        }
        // Collect the return value
        if (Source == ExitSite->getReturnValue()) {
          Res.insert(CallSite);
        }
        // Collect all relevant alloca's to map into caller context
        std::set<IDETypeStateAnalysis::d_t> RelAllocas;
        for (const auto *Fact : Res) {
          auto Allocas = Analysis->getRelevantAllocas(Fact);
          RelAllocas.insert(Allocas.begin(), Allocas.end());
        }
        Res.insert(RelAllocas.begin(), RelAllocas.end());
        return Res;
      }
      return {Source};
    }
  };
  return make_shared<TSFlowFunction>(llvm::cast<llvm::CallBase>(CallSite),
                                     CalleeFun, ExitStmt, this);
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::n_t /*RetSite*/,
    set<IDETypeStateAnalysis::f_t> Callees) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (TSD.isFactoryFunction(DemangledFname)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        IDETypeStateAnalysis::d_t CS, ZeroValue;

        TSFlowFunction(IDETypeStateAnalysis::d_t CS,
                       IDETypeStateAnalysis::d_t Z)
            : CS(CS), ZeroValue(Z) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t Source) override {
          if (Source == CS) {
            return {};
          }
          if (Source == ZeroValue) {
            return {Source, CS};
          }
          return {Source};
        }
      };
      return make_shared<TSFlowFunction>(CS, getZeroValue());
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
    if (!TSD.isAPIFunction(DemangledFname) && !Callee->isDeclaration()) {
      for (const auto &Arg : CS->args()) {
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

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getSummaryFlowFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::f_t /*DestFun*/) {
  return nullptr;
}

InitialSeeds<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
             IDETypeStateAnalysis::l_t>
IDETypeStateAnalysis::initialSeeds() {
  // just start in main()
  InitialSeeds<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
               IDETypeStateAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue(), bottomElement());
  }
  return Seeds;
}

IDETypeStateAnalysis::d_t IDETypeStateAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETypeStateAnalysis::isZeroValue(IDETypeStateAnalysis::d_t Fact) const {
  return LLVMZeroValue::isLLVMZeroValue(Fact);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t Curr, IDETypeStateAnalysis::d_t CurrNode,
    IDETypeStateAnalysis::n_t /*Succ*/, IDETypeStateAnalysis::d_t SuccNode) {
  // Set alloca instructions of target type to uninitialized.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      if (CurrNode == getZeroValue() && SuccNode == Alloca) {
        struct TSAllocaEF : public TSConstant {
          const llvm::AllocaInst *Alloca;
          TSAllocaEF(const TypeStateDescription &Tsd,
                     const llvm::AllocaInst *Alloca)
              : TSConstant(Tsd, Tsd.uninit()), Alloca(Alloca) {}

          void print(llvm::raw_ostream &OS,
                     bool /*IsForDebug = false*/) const override {
            OS << "Alloca(" << llvmIRToShortString(Alloca) << ")";
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
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::d_t /*SrcNode*/,
    IDETypeStateAnalysis::f_t /*DestinationFunction*/,
    IDETypeStateAnalysis::d_t /*DestNode*/) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::f_t /*CalleeFunction*/,
    IDETypeStateAnalysis::n_t /*ExitSite*/,
    IDETypeStateAnalysis::d_t /*ExitNode*/,
    IDETypeStateAnalysis::n_t /*ReSite*/,
    IDETypeStateAnalysis::d_t /*RetNode*/) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getCallToRetEdgeFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::d_t CallNode,
    IDETypeStateAnalysis::n_t /*RetSite*/,
    IDETypeStateAnalysis::d_t RetSiteNode,
    std::set<IDETypeStateAnalysis::f_t> Callees) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());

    // For now we assume that we can only generate from the return value.
    // We apply the same edge function for the return value, i.e. callsite.
    if (TSD.isFactoryFunction(DemangledFname)) {
      PHASAR_LOG_LEVEL(DEBUG, "Processing factory function");
      if (isZeroValue(CallNode) && RetSiteNode == CS) {
        struct TSFactoryEF : public TSConstant {
          TSFactoryEF(const TypeStateDescription &Tsd, l_t State)
              : TSConstant(Tsd, State) {}
        };
        return make_shared<TSFactoryEF>(
            TSD, TSD.getNextState(DemangledFname, TSD.uninit(), CS));
      }
    }

    // For every consuming parameter and all its aliases and relevant alloca's
    // we apply the same edge function.
    if (TSD.isConsumingFunction(DemangledFname)) {
      PHASAR_LOG_LEVEL(DEBUG, "Processing consuming function");
      for (auto Idx : TSD.getConsumerParamIdx(DemangledFname)) {
        std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas =
            getWMAliasesAndAllocas(CS->getArgOperand(Idx));

        if (CallNode == RetSiteNode &&
            PointsToAndAllocas.find(CallNode) != PointsToAndAllocas.end()) {
          return make_shared<TSEdgeFunction>(TSD, DemangledFname, CS);
        }
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::d_t /*CallNode*/,
    IDETypeStateAnalysis::n_t /*RetSite*/,
    IDETypeStateAnalysis::d_t /*RetSiteNode*/) {
  return nullptr;
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::l_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::l_t Lhs,
                           IDETypeStateAnalysis::l_t Rhs) {
  if (Lhs == Rhs) {
    return Lhs;
  }
  if (Lhs == TOP) {
    return Rhs;
  }
  if (Rhs == TOP) {
    return Lhs;
  }
  return BOTTOM;
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::l_t>>(TOP);
}

void IDETypeStateAnalysis::printNode(llvm::raw_ostream &OS, n_t Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IDETypeStateAnalysis::printDataFlowFact(llvm::raw_ostream &OS,
                                             d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IDETypeStateAnalysis::printFunction(llvm::raw_ostream &OS,
                                         IDETypeStateAnalysis::f_t Func) const {
  OS << Func->getName();
}

void IDETypeStateAnalysis::printEdgeFact(llvm::raw_ostream &OS,
                                         IDETypeStateAnalysis::l_t L) const {
  OS << TSD.stateToString(L);
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(BotElement);
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::TSEdgeFunction::computeTarget(
    IDETypeStateAnalysis::l_t Source) {

  // assert((Source != TSD.top()) && "Error: call computeTarget with TOP\n");

  auto CurrentState = TSD.getNextState(
      Token, Source == TSD.top() ? TSD.uninit() : Source, CallSite);
  PHASAR_LOG_LEVEL(DEBUG, "State machine transition: ("
                              << Token << " , " << TSD.stateToString(Source)
                              << ") -> " << TSD.stateToString(CurrentState));
  return CurrentState;
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> SecondFunction) {

  if (auto *AB = dynamic_cast<AllBottom<IDETypeStateAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *EI = dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<TSEdgeFunctionComposer>(this->shared_from_this(),
                                             SecondFunction, TSD.bottom());
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::TSEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  // if (auto *EI = dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
  //         otherFunction.get())) {
  //   return this->shared_from_this();
  // }
  if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
          OtherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(TSD.bottom());
}

bool IDETypeStateAnalysis::TSEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>> Other) const {
  /*if (auto *TSEF =
          dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(Other.get())) {
    return this->CurrentState == TSEF->CurrentState;
  }*/
  return this == Other.get();
}

void IDETypeStateAnalysis::TSEdgeFunction::print(llvm::raw_ostream &OS,
                                                 bool /*IsForDebug*/) const {
  OS << "TSEF(" << Token << " at " << llvmIRToShortString(CallSite) << ")";
}

IDETypeStateAnalysis::TSConstant::TSConstant(const TypeStateDescription &TSD,
                                             l_t State)
    : TSD(TSD), State(State) {}

auto IDETypeStateAnalysis::TSConstant::computeTarget(l_t /*Source*/) -> l_t {
  return State;
}

auto IDETypeStateAnalysis::TSConstant::composeWith(
    std::shared_ptr<EdgeFunction<l_t>> SecondFunction)
    -> std::shared_ptr<EdgeFunction<l_t>> {
  auto Ret = SecondFunction->computeTarget(State);
  if (Ret == State) {
    return shared_from_this();
  }
  if (Ret == TSD.bottom()) {
    return std::make_shared<AllBottom<l_t>>(Ret);
  }

  return std::make_shared<TSConstant>(TSD, Ret);
}

auto IDETypeStateAnalysis::TSConstant::joinWith(
    std::shared_ptr<EdgeFunction<l_t>> OtherFunction)
    -> std::shared_ptr<EdgeFunction<l_t>> {
  if (&*OtherFunction == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }

  if (auto *AT =
          dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(&*OtherFunction)) {
    return this->shared_from_this();
  }

  if (auto *C = dynamic_cast<TSConstant *>(&*OtherFunction)) {
    if (C->State == State || C->State == TSD.top()) {
      return shared_from_this();
    }
    if (State == TSD.top()) {
      return OtherFunction;
    }
  }
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(TSD.bottom());
}

bool IDETypeStateAnalysis::TSConstant::equal_to(
    std::shared_ptr<EdgeFunction<l_t>> Other) const {
  if (this == &*Other) {
    return true;
  }
  if (const auto *OtherC = dynamic_cast<TSConstant *>(&*Other)) {
    return State == OtherC->State;
  }

  return false;
}

void IDETypeStateAnalysis::TSConstant::print(llvm::raw_ostream &OS,
                                             bool /*IsForDebug*/) const {
  OS << "TSConstant[" << TSD.stateToString(State) << "]";
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getRelevantAllocas(IDETypeStateAnalysis::d_t V) {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  }
  auto PointsToSet = getWMPointsToSet(V);
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas;
  PHASAR_LOG_LEVEL(DEBUG, "Compute relevant alloca's of "
                              << IDETypeStateAnalysis::DtoString(V));
  for (const auto *Alias : PointsToSet) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "Alias: " << IDETypeStateAnalysis::DtoString(Alias));
    // Collect the pointer operand of a aliased load instruciton
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
      if (hasMatchingType(Alias)) {
        PHASAR_LOG_LEVEL(DEBUG,
                         " -> Alloca: " << IDETypeStateAnalysis::DtoString(
                             Load->getPointerOperand()));
        RelevantAllocas.insert(Load->getPointerOperand());
      }
    } else {
      // For all other types of aliases, e.g. callsites, function arguments,
      // we check store instructions where thoses aliases are value operands.
      for (const auto *User : Alias->users()) {
        PHASAR_LOG_LEVEL(DEBUG,
                         "  User: " << IDETypeStateAnalysis::DtoString(User));
        if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
          if (hasMatchingType(Store)) {
            PHASAR_LOG_LEVEL(
                DEBUG, "    -> Alloca: " << IDETypeStateAnalysis::DtoString(
                           Store->getPointerOperand()));
            RelevantAllocas.insert(Store->getPointerOperand());
          }
        }
      }
    }
  }
  for (const auto *Alias : PointsToSet) {
    RelevantAllocaCache[Alias] = RelevantAllocas;
  }
  return RelevantAllocas;
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMPointsToSet(IDETypeStateAnalysis::d_t V) {
  if (PointsToCache.find(V) != PointsToCache.end()) {
    std::set<IDETypeStateAnalysis::d_t> PointsToSet(PointsToCache[V].begin(),
                                                    PointsToCache[V].end());
    return PointsToSet;
  }
  auto PTS = PT->getPointsToSet(V);
  for (const auto *Alias : *PTS) {
    if (hasMatchingType(Alias)) {
      PointsToCache[Alias] = *PTS;
    }
  }
  std::set<IDETypeStateAnalysis::d_t> PointsToSet(PTS->begin(), PTS->end());
  return PointsToSet;
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
                                                const std::string & /*Fname*/) {
  std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas;
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas = getRelevantAllocas(V);
  std::set<IDETypeStateAnalysis::d_t>
      Aliases; // =
               // IRDB->getPointsToGraph(Fname)->getPointsToSet(V);
  for (const auto *Alias : Aliases) {
    if (hasMatchingType(Alias)) {
      PointsToAndAllocas.insert(Alias);
    }
  }
  // PointsToAndAllocas.insert(Aliases.begin(), Aliases.end());
  PointsToAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return PointsToAndAllocas;
}
bool hasMatchingTypeName(const llvm::Type *Ty, const std::string &Pattern) {
  if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty)) {
    return StructTy->getName().contains(Pattern);
  }
  // primitive type
  std::string Str;
  llvm::raw_string_ostream S(Str);
  S << *Ty;
  S.flush();
  return Str.find(Pattern) != std::string::npos;
}
bool IDETypeStateAnalysis::hasMatchingType(IDETypeStateAnalysis::d_t V) {
  // General case
  if (V->getType()->isPointerTy()) {
    if (hasMatchingTypeName(V->getType()->getPointerElementType(),
                            TSD.getTypeNameOfInterest())) {
      return true;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Alloca->getAllocatedType()->getPointerElementType(),
              TSD.getTypeNameOfInterest())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getPointerOperand()
            ->getType()
            ->getPointerElementType()
            ->isPointerTy()) {
      if (hasMatchingTypeName(Load->getPointerOperand()
                                  ->getType()
                                  ->getPointerElementType()
                                  ->getPointerElementType(),
                              TSD.getTypeNameOfInterest())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Store->getValueOperand()->getType()->getPointerElementType(),
              TSD.getTypeNameOfInterest())) {
        return true;
      }
    }
    return false;
  }
  return false;
}

void IDETypeStateAnalysis::emitTextReport(
    const SolverResults<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                        IDETypeStateAnalysis::l_t> &SR,
    llvm::raw_ostream &OS) {
  OS << "\n======= TYPE STATE RESULTS =======\n";
  for (const auto &F : ICF->getAllFunctions()) {
    OS << '\n' << getFunctionNameFromIR(F) << '\n';
    for (const auto &BB : *F) {
      for (const auto &I : BB) {
        auto Results = SR.resultsAt(&I, true);
        if (ICF->isExitInst(&I)) {
          OS << "\nAt exit stmt: " << NtoString(&I) << '\n';
          for (auto Res : Results) {
            if (const auto *Alloca =
                    llvm::dyn_cast<llvm::AllocaInst>(Res.first)) {
              if (Res.second == TSD.error()) {
                OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(Res.first) << '\n';
                for (const auto *Pred : ICF->getPredsOf(&I)) {
                  OS << "\nPredecessor: " << NtoString(Pred) << '\n';
                  auto PredResults = SR.resultsAt(Pred, true);
                  for (auto Res : PredResults) {
                    if (Res.first == Alloca) {
                      OS << "Pred State: " << LtoString(Res.second) << '\n';
                    }
                  }
                }
                OS << "============================\n";
              } else {
                OS << "\nAlloca : " << DtoString(Res.first)
                   << "\nState  : " << LtoString(Res.second) << '\n';
              }
            } else {
              OS << "\nInst: " << NtoString(&I) << '\n'
                 << "Fact: " << DtoString(Res.first) << '\n'
                 << "State: " << LtoString(Res.second) << '\n';
            }
          }
        } else {
          for (auto Res : Results) {
            if (const auto *Alloca =
                    llvm::dyn_cast<llvm::AllocaInst>(Res.first)) {
              if (Res.second == TSD.error()) {
                OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(Res.first) << '\n'
                   << "\nAt IR Inst: " << NtoString(&I) << '\n';
                for (const auto *Pred : ICF->getPredsOf(&I)) {
                  OS << "\nPredecessor: " << NtoString(Pred) << '\n';
                  auto PredResults = SR.resultsAt(Pred, true);
                  for (auto Res : PredResults) {
                    if (Res.first == Alloca) {
                      OS << "Pred State: " << LtoString(Res.second) << '\n';
                    }
                  }
                }
                OS << "============================\n";
              }
            } else {
              OS << "\nInst: " << NtoString(&I) << '\n'
                 << "Fact: " << DtoString(Res.first) << '\n'
                 << "State: " << LtoString(Res.second) << '\n';
            }
          }
        }
      }
    }
    OS << "\n--------------------------------------------\n";
  }
}

} // namespace psr
