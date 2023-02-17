/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <utility>

namespace psr {

// customize the edge function composer
class TSEdgeFunctionComposer
    : public EdgeFunctionComposer<IDETypeStateAnalysisDomain ::l_t> {
private:
  IDETypeStateAnalysisDomain ::l_t BotElement;

public:
  TSEdgeFunctionComposer(
      std::shared_ptr<EdgeFunction<IDETypeStateAnalysisDomain ::l_t>> F,
      std::shared_ptr<EdgeFunction<IDETypeStateAnalysisDomain ::l_t>> G,
      IDETypeStateAnalysisDomain ::l_t Bot)
      : EdgeFunctionComposer<
            IDETypeStateAnalysisDomain ::IDETypeStateAnalysisDomain ::l_t>(F,
                                                                           G),
        BotElement(Bot) {}

  std::shared_ptr<EdgeFunction<IDETypeStateAnalysisDomain ::l_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDETypeStateAnalysisDomain ::l_t>>
               OtherFunction) override {
    if (OtherFunction.get() == this ||
        OtherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }
    if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
            OtherFunction.get())) {
      return this->shared_from_this();
    }
    return std::make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(BotElement);
  }
};

class TSEdgeFunction : public EdgeFunction<IDETypeStateAnalysisDomain ::l_t>,
                       public std::enable_shared_from_this<TSEdgeFunction> {
protected:
  const TypeStateDescription *TSD;
  // Do not use a reference here, since LLVM's StringRef's (obtained by str())
  // might turn to nullptr for whatever reason...
  const std::string Token;
  const llvm::CallBase *CallSite;

public:
  using l_t = IDETypeStateAnalysisDomain ::l_t;

  TSEdgeFunction(const TypeStateDescription *TSD, const std::string &Tok,
                 const llvm::CallBase *CB)
      : TSD(TSD), Token(Tok), CallSite(CB){};

  l_t computeTarget(l_t Source) override {

    // assert((Source != TSD->top()) && "Error: call computeTarget with TOP\n");

    auto CurrentState = TSD->getNextState(
        Token, Source == TSD->top() ? TSD->uninit() : Source, CallSite);
    PHASAR_LOG_LEVEL(DEBUG, "State machine transition: ("
                                << Token << " , " << TSD->stateToString(Source)
                                << ") -> " << TSD->stateToString(CurrentState));
    return CurrentState;
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> SecondFunction) override {

    if (auto *AB = dynamic_cast<AllBottom<IDETypeStateAnalysis::l_t> *>(
            SecondFunction.get())) {
      return SecondFunction;
    }
    if (auto *EI = dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
            SecondFunction.get())) {
      return this->shared_from_this();
    }
    return std::make_shared<TSEdgeFunctionComposer>(
        this->shared_from_this(), SecondFunction, TSD->bottom());
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> OtherFunction) override {
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
    return std::make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(
        TSD->bottom());
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> Other) const override {
    /*if (auto *TSEF =
           dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(Other.get())) {
     return this->CurrentState == TSEF->CurrentState;
   }*/
    return this == Other.get();
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug*/ = false) const override {
    OS << "TSEF(" << Token << " at " << llvmIRToShortString(CallSite) << ")";
  }
};

class TSConstant : public EdgeFunction<IDETypeStateAnalysisDomain ::l_t>,
                   public std::enable_shared_from_this<TSConstant> {
  const TypeStateDescription *TSD;
  IDETypeStateAnalysisDomain ::l_t State;

public:
  using l_t = IDETypeStateAnalysisDomain ::l_t;
  explicit TSConstant(const TypeStateDescription *TSD, l_t State)
      : TSD(TSD), State(State) {}

  l_t computeTarget(l_t /*Source*/) override { return State; }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> SecondFunction) override {
    auto Ret = SecondFunction->computeTarget(State);
    if (Ret == State) {
      return shared_from_this();
    }
    if (Ret == TSD->bottom()) {
      return std::make_shared<AllBottom<l_t>>(Ret);
    }

    return std::make_shared<TSConstant>(TSD, Ret);
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> OtherFunction) override {
    if (&*OtherFunction == this ||
        OtherFunction->equal_to(this->shared_from_this())) {
      return this->shared_from_this();
    }

    if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
            &*OtherFunction)) {
      return this->shared_from_this();
    }

    if (auto *C = dynamic_cast<TSConstant *>(&*OtherFunction)) {
      if (C->State == State || C->State == TSD->top()) {
        return shared_from_this();
      }
      if (State == TSD->top()) {
        return OtherFunction;
      }
    }
    return std::make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(
        TSD->bottom());
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> Other) const override {
    if (this == &*Other) {
      return true;
    }
    if (const auto *OtherC = dynamic_cast<TSConstant *>(&*Other)) {
      return State == OtherC->State;
    }

    return false;
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug*/ = false) const override {
    OS << "TSConstant[" << TSD->stateToString(State) << "]";
  }
};

IDETypeStateAnalysis::IDETypeStateAnalysis(const LLVMProjectIRDB *IRDB,
                                           LLVMAliasInfoRef PT,
                                           const TypeStateDescription *TSD,
                                           std::vector<std::string> EntryPoints)
    : IDETabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      TSD(TSD), PT(PT) {
  assert(TSD != nullptr);
  assert(PT);
}

// Start formulating our analysis by specifying the parts required for IFDS

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getNormalFlowFunction(
    IDETypeStateAnalysis::n_t Curr, IDETypeStateAnalysis::n_t /*Succ*/) {
  // Check if Alloca's type matches the target type. If so, generate from zero
  // value.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      return generateFromZero(Alloca);
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
        std::set<IDETypeStateAnalysis::d_t>
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
      return std::make_shared<TSFlowFunction>(Load);
    }
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    if (hasMatchingType(Gep->getPointerOperand())) {
      return lambdaFlow<d_t>([=](d_t Source) -> std::set<d_t> {
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
        std::set<IDETypeStateAnalysis::d_t>
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
      return std::make_shared<TSFlowFunction>(Store, RelevantAliasesAndAllocas);
    }
  }
  return Identity<IDETypeStateAnalysis::d_t>::getInstance();
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t CallSite,
                                          IDETypeStateAnalysis::f_t DestFun) {
  // Kill all data-flow facts if we hit a function of the target API.
  // Those functions are modled within Call-To-Return.
  if (TSD->isAPIFunction(llvm::demangle(DestFun->getName().str()))) {
    return killAllFlows<d_t>();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping.
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return mapFactsToCallee(Call, DestFun);
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

    std::set<IDETypeStateAnalysis::d_t>
    computeTargets(IDETypeStateAnalysis::d_t Source) override {
      if (!LLVMZeroValue::isLLVMZeroValue(Source)) {
        std::set<const llvm::Value *> Res;
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
  return std::make_shared<TSFlowFunction>(llvm::cast<llvm::CallBase>(CallSite),
                                          CalleeFun, ExitStmt, this);
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::n_t /*RetSite*/,
    llvm::ArrayRef<f_t> Callees) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (TSD->isFactoryFunction(DemangledFname)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        IDETypeStateAnalysis::d_t CS, ZeroValue;

        TSFlowFunction(IDETypeStateAnalysis::d_t CS,
                       IDETypeStateAnalysis::d_t Z)
            : CS(CS), ZeroValue(Z) {}
        ~TSFlowFunction() override = default;
        std::set<IDETypeStateAnalysis::d_t>
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
      return std::make_shared<TSFlowFunction>(CS, getZeroValue());
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
    if (!TSD->isAPIFunction(DemangledFname) && !Callee->isDeclaration()) {
      for (const auto &Arg : CS->args()) {
        if (hasMatchingType(Arg)) {
          return killManyFlows<d_t>(getWMAliasesAndAllocas(Arg.get()));
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
    Seeds.addSeed(&IRDB->getFunction(EntryPoint)->front().front(),
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

auto IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t Curr, IDETypeStateAnalysis::d_t CurrNode,
    IDETypeStateAnalysis::n_t /*Succ*/, IDETypeStateAnalysis::d_t SuccNode)
    -> EdgeFunctionPtrType {
  // Set alloca instructions of target type to uninitialized.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      if (CurrNode == getZeroValue() && SuccNode == Alloca) {
        struct TSAllocaEF : public TSConstant {
          const llvm::AllocaInst *Alloca;
          TSAllocaEF(const TypeStateDescription *Tsd,
                     const llvm::AllocaInst *Alloca)
              : TSConstant(Tsd, Tsd->uninit()), Alloca(Alloca) {}

          void print(llvm::raw_ostream &OS,
                     bool /*IsForDebug = false*/) const override {
            OS << "Alloca(" << llvmIRToShortString(Alloca) << ")";
          }
        };
        return std::make_shared<TSAllocaEF>(TSD, Alloca);
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

auto IDETypeStateAnalysis::getCallEdgeFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::d_t /*SrcNode*/,
    IDETypeStateAnalysis::f_t /*DestinationFunction*/,
    IDETypeStateAnalysis::d_t /*DestNode*/) -> EdgeFunctionPtrType {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

auto IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::f_t /*CalleeFunction*/,
    IDETypeStateAnalysis::n_t /*ExitSite*/,
    IDETypeStateAnalysis::d_t /*ExitNode*/,
    IDETypeStateAnalysis::n_t /*ReSite*/, IDETypeStateAnalysis::d_t /*RetNode*/)
    -> EdgeFunctionPtrType {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

auto IDETypeStateAnalysis::getCallToRetEdgeFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::d_t CallNode,
    IDETypeStateAnalysis::n_t /*RetSite*/,
    IDETypeStateAnalysis::d_t RetSiteNode, llvm::ArrayRef<f_t> Callees)
    -> EdgeFunctionPtrType {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());

    // For now we assume that we can only generate from the return value.
    // We apply the same edge function for the return value, i.e. callsite.
    if (TSD->isFactoryFunction(DemangledFname)) {
      PHASAR_LOG_LEVEL(DEBUG, "Processing factory function");
      if (isZeroValue(CallNode) && RetSiteNode == CS) {
        struct TSFactoryEF : public TSConstant {
          TSFactoryEF(const TypeStateDescription *Tsd, l_t State)
              : TSConstant(Tsd, State) {}
        };
        return std::make_shared<TSFactoryEF>(
            TSD, TSD->getNextState(DemangledFname, TSD->uninit(), CS));
      }
    }

    // For every consuming parameter and all its aliases and relevant alloca's
    // we apply the same edge function.
    if (TSD->isConsumingFunction(DemangledFname)) {
      PHASAR_LOG_LEVEL(DEBUG, "Processing consuming function");
      for (auto Idx : TSD->getConsumerParamIdx(DemangledFname)) {
        std::set<IDETypeStateAnalysis::d_t> AliasAndAllocas =
            getWMAliasesAndAllocas(CS->getArgOperand(Idx));

        if (CallNode == RetSiteNode &&
            AliasAndAllocas.find(CallNode) != AliasAndAllocas.end()) {
          return std::make_shared<TSEdgeFunction>(TSD, DemangledFname, CS);
        }
      }
    }
  }
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

auto IDETypeStateAnalysis::getSummaryEdgeFunction(
    IDETypeStateAnalysis::n_t /*CallSite*/,
    IDETypeStateAnalysis::d_t /*CallNode*/,
    IDETypeStateAnalysis::n_t /*RetSite*/,
    IDETypeStateAnalysis::d_t /*RetSiteNode*/) -> EdgeFunctionPtrType {
  return nullptr;
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::topElement() {
  return TSD->top();
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::bottomElement() {
  return TSD->bottom();
}

IDETypeStateAnalysis::l_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::l_t Lhs,
                           IDETypeStateAnalysis::l_t Rhs) {
  if (Lhs == Rhs) {
    return Lhs;
  }
  if (Lhs == TSD->top()) {
    return Rhs;
  }
  if (Rhs == TSD->top()) {
    return Lhs;
  }
  return TSD->bottom();
}

std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::allTopFunction() {
  return std::make_shared<AllTop<IDETypeStateAnalysis::l_t>>(TSD->top());
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
  OS << TSD->stateToString(L);
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getRelevantAllocas(IDETypeStateAnalysis::d_t V) {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  }
  auto AliasSet = getWMAliasSet(V);
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas;
  PHASAR_LOG_LEVEL(DEBUG, "Compute relevant alloca's of "
                              << IDETypeStateAnalysis::DtoString(V));
  for (const auto *Alias : AliasSet) {
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
  for (const auto *Alias : AliasSet) {
    RelevantAllocaCache[Alias] = RelevantAllocas;
  }
  return RelevantAllocas;
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMAliasSet(IDETypeStateAnalysis::d_t V) {
  if (AliasCache.find(V) != AliasCache.end()) {
    std::set<IDETypeStateAnalysis::d_t> AliasSet(AliasCache[V].begin(),
                                                 AliasCache[V].end());
    return AliasSet;
  }
  auto PTS = PT.getAliasSet(V);
  for (const auto *Alias : *PTS) {
    if (hasMatchingType(Alias)) {
      AliasCache[Alias] = *PTS;
    }
  }
  std::set<IDETypeStateAnalysis::d_t> AliasSet(PTS->begin(), PTS->end());
  return AliasSet;
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMAliasesAndAllocas(IDETypeStateAnalysis::d_t V) {
  std::set<IDETypeStateAnalysis::d_t> AliasAndAllocas;
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas = getRelevantAllocas(V);
  std::set<IDETypeStateAnalysis::d_t> Aliases = getWMAliasSet(V);
  AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getLocalAliasesAndAllocas(IDETypeStateAnalysis::d_t V,
                                                const std::string & /*Fname*/) {
  std::set<IDETypeStateAnalysis::d_t> AliasAndAllocas;
  std::set<IDETypeStateAnalysis::d_t> RelevantAllocas = getRelevantAllocas(V);
  std::set<IDETypeStateAnalysis::d_t>
      Aliases; // =
               // IRDB->getAliasGraph(Fname)->getAliasSet(V);
  for (const auto *Alias : Aliases) {
    if (hasMatchingType(Alias)) {
      AliasAndAllocas.insert(Alias);
    }
  }
  // AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
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
                            TSD->getTypeNameOfInterest())) {
      return true;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Alloca->getAllocatedType()->getPointerElementType(),
              TSD->getTypeNameOfInterest())) {
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
                              TSD->getTypeNameOfInterest())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Store->getValueOperand()->getType()->getPointerElementType(),
              TSD->getTypeNameOfInterest())) {
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

  LLVMBasedCFG CFG;
  OS << "\n======= TYPE STATE RESULTS =======\n";
  for (const auto &F : IRDB->getAllFunctions()) {
    OS << '\n' << getFunctionNameFromIR(F) << '\n';
    for (const auto &BB : *F) {
      for (const auto &I : BB) {
        auto Results = SR.resultsAt(&I, true);
        if (CFG.isExitInst(&I)) {
          OS << "\nAt exit stmt: " << NtoString(&I) << '\n';
          for (auto Res : Results) {
            if (const auto *Alloca =
                    llvm::dyn_cast<llvm::AllocaInst>(Res.first)) {
              if (Res.second == TSD->error()) {
                OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(Res.first) << '\n';
                for (const auto *Pred : CFG.getPredsOf(&I)) {
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
              if (Res.second == TSD->error()) {
                OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                   << DtoString(Res.first) << '\n'
                   << "\nAt IR Inst: " << NtoString(&I) << '\n';
                for (const auto *Pred : CFG.getPredsOf(&I)) {
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