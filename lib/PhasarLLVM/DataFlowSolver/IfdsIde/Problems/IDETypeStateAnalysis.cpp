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

#include "llvm/IR/CallSite.h"
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
  IDETabulationProblem::ZeroValue = createZeroValue();
}

// Start formulating our analysis by specifying the parts required for IFDS

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getNormalFlowFunction(IDETypeStateAnalysis::n_t Curr,
                                            IDETypeStateAnalysis::n_t Succ) {
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
      return make_shared<LambdaFlow<d_t>>([=](d_t Source) -> set<d_t> {
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
IDETypeStateAnalysis::getCallFlowFunction(IDETypeStateAnalysis::n_t CallStmt,
                                          IDETypeStateAnalysis::f_t DestFun) {
  // Kill all data-flow facts if we hit a function of the target API.
  // Those functions are modled within Call-To-Return.
  if (TSD.isAPIFunction(cxxDemangle(DestFun->getName().str()))) {
    return KillAll<IDETypeStateAnalysis::d_t>::getInstance();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping.
  if (llvm::isa<llvm::CallInst>(CallStmt) ||
      llvm::isa<llvm::InvokeInst>(CallStmt)) {
    return make_shared<MapFactsToCallee<>>(llvm::ImmutableCallSite(CallStmt),
                                           DestFun);
  }
  llvm::report_fatal_error("callStmt not a CallInst nor a InvokeInst");
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getRetFlowFunction(IDETypeStateAnalysis::n_t CallSite,
                                         IDETypeStateAnalysis::f_t CalleeFun,
                                         IDETypeStateAnalysis::n_t ExitStmt,
                                         IDETypeStateAnalysis::n_t RetSite) {
  // Besides mapping the formal parameter back into the actual parameter and
  // propagating the return value into the caller context, we also propagate
  // all related alloca's of the formal parameter and the return value.
  struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
    llvm::ImmutableCallSite CallSite;
    const llvm::Function *CalleeFun;
    const llvm::ReturnInst *ExitStmt;
    IDETypeStateAnalysis *Analysis;
    std::vector<const llvm::Value *> Actuals;
    std::vector<const llvm::Value *> Formals;
    TSFlowFunction(llvm::ImmutableCallSite CS, const llvm::Function *CalleeFun,
                   const llvm::Instruction *ExitStmt,
                   IDETypeStateAnalysis *Analysis)
        : CallSite(CS), CalleeFun(CalleeFun),
          ExitStmt(llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)),
          Analysis(Analysis) {
      // Set up the actual parameters
      for (unsigned Idx = 0; Idx < CallSite.getNumArgOperands(); ++Idx) {
        Actuals.push_back(CallSite.getArgOperand(Idx));
      }
      // Set up the formal parameters
      for (unsigned Idx = 0; Idx < CalleeFun->arg_size(); ++Idx) {
        Formals.push_back(getNthFunctionArgument(CalleeFun, Idx));
      }
    }

    ~TSFlowFunction() override = default;

    set<IDETypeStateAnalysis::d_t>
    computeTargets(IDETypeStateAnalysis::d_t Source) override {
      if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
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
        if (Source == ExitStmt->getReturnValue()) {
          Res.insert(CallSite.getInstruction());
        }
        // Collect all relevant alloca's to map into caller context
        std::set<IDETypeStateAnalysis::d_t> RelAllocas;
        for (const auto *Fact : Res) {
          auto Allocas = Analysis->getRelevantAllocas(Fact);
          RelAllocas.insert(Allocas.begin(), Allocas.end());
        }
        Res.insert(RelAllocas.begin(), RelAllocas.end());
        return Res;
      } else {
        return {Source};
      }
    }
  };
  return make_shared<TSFlowFunction>(llvm::ImmutableCallSite(CallSite),
                                     CalleeFun, ExitStmt, this);
}

IDETypeStateAnalysis::FlowFunctionPtrType
IDETypeStateAnalysis::getCallToRetFlowFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::n_t RetSite,
    set<IDETypeStateAnalysis::f_t> Callees) {
  const llvm::ImmutableCallSite CS(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = cxxDemangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (TSD.isFactoryFunction(DemangledFname)) {
      struct TSFlowFunction : FlowFunction<IDETypeStateAnalysis::d_t> {
        IDETypeStateAnalysis::d_t CallSite, ZeroValue;

        TSFlowFunction(IDETypeStateAnalysis::d_t CS,
                       IDETypeStateAnalysis::d_t Z)
            : CallSite(CS), ZeroValue(Z) {}
        ~TSFlowFunction() override = default;
        set<IDETypeStateAnalysis::d_t>
        computeTargets(IDETypeStateAnalysis::d_t Source) override {
          if (Source == CallSite) {
            return {};
          }
          if (Source == ZeroValue) {
            return {Source, CallSite};
          }
          return {Source};
        }
      };
      return make_shared<TSFlowFunction>(CallSite, getZeroValue());
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
      for (const auto &Arg : CS.args()) {
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
    IDETypeStateAnalysis::n_t CallStmt, IDETypeStateAnalysis::f_t DestFun) {
  return nullptr;
}

map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>>
IDETypeStateAnalysis::initialSeeds() {
  // just start in main()
  map<IDETypeStateAnalysis::n_t, set<IDETypeStateAnalysis::d_t>> SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
    SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                             set<IDETypeStateAnalysis::d_t>({getZeroValue()})));
  }
  return SeedMap;
}

IDETypeStateAnalysis::d_t IDETypeStateAnalysis::createZeroValue() const {
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IDETypeStateAnalysis::isZeroValue(IDETypeStateAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

// in addition provide specifications for the IDE parts

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getNormalEdgeFunction(
    IDETypeStateAnalysis::n_t Curr, IDETypeStateAnalysis::d_t CurrNode,
    IDETypeStateAnalysis::n_t Succ, IDETypeStateAnalysis::d_t SuccNode) {
  // Set alloca instructions of target type to uninitialized.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      if (CurrNode == getZeroValue() && SuccNode == Alloca) {
        struct TSAllocaEF : public EdgeFunction<l_t>,
                            public std::enable_shared_from_this<TSAllocaEF> {
          const TypeStateDescription &TSD;
          l_t CurrentState;
          const llvm::AllocaInst *Alloca;
          TSAllocaEF(const TypeStateDescription &Tsd,
                     const llvm::AllocaInst *Alloca)
              : TSD(Tsd), CurrentState(Tsd.top()), Alloca(Alloca) {}

          IDETypeStateAnalysis::l_t
          computeTarget(IDETypeStateAnalysis::l_t Source) override {
            // std::cerr << "UNINIT INITIALIZATION: " << llvmIRToString(Alloca)
            //          << std::endl;
            CurrentState = TSD.uninit();
            return CurrentState;
          }

          void print(std::ostream &OS, bool IsForDebug = false) const override {
            OS << "Alloca(" << TSD.stateToString(CurrentState) << ")";
          }

          bool equal_to(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                            Other) const override {
            if (auto *TSEF =
                    dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(
                        Other.get())) {
              return this->CurrentState == TSEF->getCurrentState();
            }
            return this == Other.get();
          }
          std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
          composeWith(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                          SecondFunction) override {
            if (auto *AB = dynamic_cast<AllBottom<IDETypeStateAnalysis::l_t> *>(
                    SecondFunction.get())) {
              return this->shared_from_this();
            }
            if (auto *EI =
                    dynamic_cast<EdgeIdentity<IDETypeStateAnalysis::l_t> *>(
                        SecondFunction.get())) {
              return this->shared_from_this();
            }
            return make_shared<TSEdgeFunctionComposer>(
                this->shared_from_this(), SecondFunction, TSD.bottom());
          }

          std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
          joinWith(std::shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
                       OtherFunction) override {
            if (OtherFunction.get() == this ||
                OtherFunction->equal_to(this->shared_from_this())) {
              return this->shared_from_this();
            }
            if (auto *AT = dynamic_cast<AllTop<IDETypeStateAnalysis::l_t> *>(
                    OtherFunction.get())) {
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
    IDETypeStateAnalysis::n_t CallStmt, IDETypeStateAnalysis::d_t SrcNode,
    IDETypeStateAnalysis::f_t DestinationFunction,
    IDETypeStateAnalysis::d_t DestNode) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getReturnEdgeFunction(
    IDETypeStateAnalysis::n_t CallSite,
    IDETypeStateAnalysis::f_t CalleeFunction,
    IDETypeStateAnalysis::n_t ExitStmt, IDETypeStateAnalysis::d_t ExitNode,
    IDETypeStateAnalysis::n_t ReSite, IDETypeStateAnalysis::d_t RetNode) {
  return EdgeIdentity<IDETypeStateAnalysis::l_t>::getInstance();
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::getCallToRetEdgeFunction(
    IDETypeStateAnalysis::n_t CallSite, IDETypeStateAnalysis::d_t CallNode,
    IDETypeStateAnalysis::n_t RetSite, IDETypeStateAnalysis::d_t RetSiteNode,
    std::set<IDETypeStateAnalysis::f_t> Callees) {
  const llvm::ImmutableCallSite CS(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = cxxDemangle(Callee->getName().str());

    // For now we assume that we can only generate from the return value.
    // We apply the same edge function for the return value, i.e. callsite.
    if (TSD.isFactoryFunction(DemangledFname)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Processing factory function");
      if (isZeroValue(CallNode) && RetSiteNode == CS.getInstruction()) {
        struct TSFactoryEF : public TSEdgeFunction {
          TSFactoryEF(const TypeStateDescription &Tsd, const std::string &Tok,
                      llvm::ImmutableCallSite Cs)
              : TSEdgeFunction(Tsd, Tok, Cs) {}

          IDETypeStateAnalysis::l_t
          computeTarget(IDETypeStateAnalysis::l_t Source) override {
            // CurrentState = TSD.start();
            CurrentState = TSD.getNextState(
                Token, Source == TSD.top() ? TSD.uninit() : Source, CS);
            return CurrentState;
          }

          void print(std::ostream &OS, bool IsForDebug = false) const override {
            OS << "Factory(" << TSD.stateToString(CurrentState) << ")";
          }
        };
        return make_shared<TSFactoryEF>(TSD, DemangledFname, CS);
      }
    }

    // For every consuming parameter and all its aliases and relevant alloca's
    // we apply the same edge function.
    if (TSD.isConsumingFunction(DemangledFname)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Processing consuming function");
      for (auto Idx : TSD.getConsumerParamIdx(DemangledFname)) {
        std::set<IDETypeStateAnalysis::d_t> PointsToAndAllocas =
            getWMAliasesAndAllocas(CS.getArgument(Idx));

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
    IDETypeStateAnalysis::n_t CallStmt, IDETypeStateAnalysis::d_t CallNode,
    IDETypeStateAnalysis::n_t RetSite, IDETypeStateAnalysis::d_t RetSiteNode) {
  return nullptr;
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::topElement() { return TOP; }

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::bottomElement() {
  return BOTTOM;
}

IDETypeStateAnalysis::l_t
IDETypeStateAnalysis::join(IDETypeStateAnalysis::l_t Lhs,
                           IDETypeStateAnalysis::l_t Rhs) {
  if (Lhs == TOP && Rhs != BOTTOM) {
    return Rhs;
  } else if (Rhs == TOP && Lhs != BOTTOM) {
    return Lhs;
  } else if (Lhs == Rhs) {
    return Lhs;
  } else {
    return BOTTOM;
  }
}

shared_ptr<EdgeFunction<IDETypeStateAnalysis::l_t>>
IDETypeStateAnalysis::allTopFunction() {
  return make_shared<AllTop<IDETypeStateAnalysis::l_t>>(TOP);
}

void IDETypeStateAnalysis::printNode(std::ostream &OS, n_t N) const {
  OS << llvmIRToString(N);
}

void IDETypeStateAnalysis::printDataFlowFact(std::ostream &OS, d_t D) const {
  OS << llvmIRToString(D);
}

void IDETypeStateAnalysis::printFunction(ostream &OS,
                                         IDETypeStateAnalysis::f_t M) const {
  OS << M->getName().str();
}

void IDETypeStateAnalysis::printEdgeFact(ostream &OS,
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
  return make_shared<AllBottom<IDETypeStateAnalysis::l_t>>(botElement);
}

IDETypeStateAnalysis::l_t IDETypeStateAnalysis::TSEdgeFunction::computeTarget(
    IDETypeStateAnalysis::l_t Source) {
  CurrentState = TSD.getNextState(Token, Source);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "State machine transition: (" << Token << " , "
                << TSD.stateToString(Source) << ") -> "
                << TSD.stateToString(CurrentState));
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
  if (auto *TSEF =
          dynamic_cast<IDETypeStateAnalysis::TSEdgeFunction *>(Other.get())) {
    return this->CurrentState == TSEF->CurrentState;
  }
  return this == Other.get();
}

void IDETypeStateAnalysis::TSEdgeFunction::print(ostream &OS,
                                                 bool IsForDebug) const {
  OS << "TSEF(" << TSD.stateToString(CurrentState) << ")";
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getRelevantAllocas(IDETypeStateAnalysis::d_t V) {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  } else {
    auto PointsToSet = getWMPointsToSet(V);
    std::set<IDETypeStateAnalysis::d_t> RelevantAllocas;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Compute relevant alloca's of "
                  << IDETypeStateAnalysis::DtoString(V));
    for (const auto *Alias : PointsToSet) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Alias: " << IDETypeStateAnalysis::DtoString(Alias));
      // Collect the pointer operand of a aliased load instruciton
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
        if (hasMatchingType(Alias)) {
          LOG_IF_ENABLE(
              BOOST_LOG_SEV(lg::get(), DEBUG)
              << " -> Alloca: "
              << IDETypeStateAnalysis::DtoString(Load->getPointerOperand()));
          RelevantAllocas.insert(Load->getPointerOperand());
        }
      } else {
        // For all other types of aliases, e.g. callsites, function arguments,
        // we check store instructions where thoses aliases are value operands.
        for (const auto *User : Alias->users()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "  User: " << IDETypeStateAnalysis::DtoString(User));
          if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
            if (hasMatchingType(Store)) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                            << "    -> Alloca: "
                            << IDETypeStateAnalysis::DtoString(
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
}

std::set<IDETypeStateAnalysis::d_t>
IDETypeStateAnalysis::getWMPointsToSet(IDETypeStateAnalysis::d_t V) {
  if (PointsToCache.find(V) != PointsToCache.end()) {
    std::set<IDETypeStateAnalysis::d_t> PointsToSet(PointsToCache[V].begin(),
                                                    PointsToCache[V].end());
    return PointsToSet;
  } else {
    const auto PTS = PT->getPointsToSet(V);
    for (const auto *Alias : *PTS) {
      if (hasMatchingType(Alias)) {
        PointsToCache[Alias] = *PTS;
      }
    }
    std::set<IDETypeStateAnalysis::d_t> PointsToSet(PTS->begin(), PTS->end());
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
  } else {
    // primitive type
    std::string Str;
    llvm::raw_string_ostream S(Str);
    S << *Ty;
    S.flush();
    return Str.find(Pattern) != std::string::npos;
  }
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
    std::ostream &OS) {
  OS << "\n======= TYPE STATE RESULTS =======\n";
  for (const auto &F : ICF->getAllFunctions()) {
    OS << '\n' << getFunctionNameFromIR(F) << '\n';
    for (const auto &BB : *F) {
      for (const auto &I : BB) {
        auto Results = SR.resultsAt(&I, true);
        if (ICF->isExitStmt(&I)) {
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
              OS << "\nInst: " << NtoString(&I) << endl
                 << "Fact: " << DtoString(Res.first) << endl
                 << "State: " << LtoString(Res.second) << endl;
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
              OS << "\nInst: " << NtoString(&I) << endl
                 << "Fact: " << DtoString(Res.first) << endl
                 << "State: " << LtoString(Res.second) << endl;
            }
          }
        }
      }
    }
    OS << "\n--------------------------------------------\n";
  }
}

} // namespace psr
