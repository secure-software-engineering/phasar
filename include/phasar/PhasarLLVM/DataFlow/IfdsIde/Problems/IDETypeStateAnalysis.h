/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDETYPESTATEANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDETYPESTATEANALYSIS_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;

namespace detail {
class IDETypeStateAnalysisBase {
protected:
  template <typename IsTypeNameOfInterestFn>
  IDETypeStateAnalysisBase(
      LLVMAliasInfoRef PT,
      IsTypeNameOfInterestFn &&IsTypeNameOfInterest) noexcept
      : PT(PT), IsTypeNameOfInterest(std::forward<IsTypeNameOfInterestFn>(
                    IsTypeNameOfInterest)) {}

  using d_t = const llvm::Value *;
  using container_type = std::set<d_t>;
  /**
   * @brief Returns all alloca's that are (indirect) aliases of V.
   *
   * Currently PhASAR's points-to information does not include alloca
   * instructions, since alloca instructions, i.e. memory locations, are of
   * type T* for a target type T. Thus they do not alias directly. Therefore,
   * for each alias of V we collect related alloca instructions by checking
   * load and store instructions for used alloca's.
   */
  container_type getRelevantAllocas(d_t V);

  /**
   * @brief Returns whole-module aliases of V.
   *
   * This function retrieves whole-module points-to information. We store
   * already computed points-to information in a cache to prevent expensive
   * recomputation since the whole module points-to graph can be huge. This
   * might become unnecessary once PhASAR's AliasGraph starts using a cache
   * itself.
   */
  container_type getWMAliasSet(d_t V);

  /**
   * @brief Provides whole module aliases and relevant alloca's of V.
   */
  container_type getWMAliasesAndAllocas(d_t V);

  /**
   * @brief Provides local aliases and relevant alloca's of V.
   */
  container_type getLocalAliasesAndAllocas(d_t V, llvm::StringRef Fname);

  /**
   * @brief Checks if the type machtes the type of interest.
   */
  bool hasMatchingType(d_t V);

private:
  std::map<const llvm::Value *, LLVMAliasInfo::AliasSetTy> AliasCache;
  LLVMAliasInfoRef PT{};
  std::map<const llvm::Value *, std::set<const llvm::Value *>>
      RelevantAllocaCache;
  std::function<bool(llvm::StringRef)> IsTypeNameOfInterest;
};
} // namespace detail

template <typename TypeStateDescriptionTy>
struct IDETypeStateAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = typename TypeStateDescriptionTy::State;
};

template <typename TypeStateDescriptionTy>
class IDETypeStateAnalysis
    : public IDETabulationProblem<
          IDETypeStateAnalysisDomain<TypeStateDescriptionTy>>,
      private detail::IDETypeStateAnalysisBase {
public:
  using IDETabProblemType =
      IDETabulationProblem<IDETypeStateAnalysisDomain<TypeStateDescriptionTy>>;
  using typename IDETabProblemType::container_type;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  using typename IDETabProblemType::FlowFunctionPtrType;
  using ConfigurationTy = TypeStateDescriptionTy;

private:
  static AllBottom<l_t>
  makeAllBottom(const TypeStateDescriptionTy *TSD) noexcept {
    if constexpr (HasJoinLatticeTraits<l_t>) {
      return AllBottom<l_t>{};
    } else {
      return AllBottom<l_t>{TSD->bottom()};
    }
  }
  template <typename LL = l_t,
            typename = std::enable_if_t<HasJoinLatticeTraits<LL>>>
  static AllBottom<l_t> makeAllBottom(EmptyType) noexcept {
    return AllBottom<l_t>{};
  }
  static bool isBottom(l_t State, const TypeStateDescriptionTy *TSD) noexcept {
    if constexpr (HasJoinLatticeTraits<l_t>) {
      return State == JoinLatticeTraits<l_t>::bottom();
    } else {
      return State == TSD->bottom();
    }
  }
  template <typename LL = l_t,
            typename = std::enable_if_t<HasJoinLatticeTraits<LL>>>
  static bool isBottom(l_t State, EmptyType) noexcept {
    return State == JoinLatticeTraits<l_t>::bottom();
  }

  struct TSEdgeFunctionComposer : EdgeFunctionComposer<l_t> {
    TSEdgeFunctionComposer(EdgeFunction<l_t> First, EdgeFunction<l_t> Second,
                           const TypeStateDescriptionTy *TSD) noexcept
        : EdgeFunctionComposer<l_t>{std::move(First), std::move(Second)} {
      if constexpr (!HasJoinLatticeTraits<l_t>) {
        BotElement = TSD->bottom();
      }
    }

    [[no_unique_address]] std::conditional_t<HasJoinLatticeTraits<l_t>,
                                             EmptyType, l_t>
        BotElement{};

    static EdgeFunction<l_t> join(EdgeFunctionRef<TSEdgeFunctionComposer> This,
                                  const EdgeFunction<l_t> &OtherFunction) {
      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }
      if constexpr (HasJoinLatticeTraits<l_t>) {
        return AllBottom<l_t>{};
      } else {
        return AllBottom<l_t>{This->BotElement};
      }
    }
  };

  struct TSEdgeFunction {
    using l_t = l_t;
    const TypeStateDescriptionTy *TSD{};
    // XXX: Do we really need a string here? Can't we just use an integer or sth
    // else that is cheap?
    std::string Token;
    const llvm::CallBase *CallSite{};

    [[nodiscard]] l_t computeTarget(l_t Source) const {

      // assert((Source != TSD->top()) && "Error: call computeTarget with
      // TOP\n");

      auto CurrentState = TSD->getNextState(
          Token, Source == TSD->top() ? TSD->uninit() : Source, CallSite);
      PHASAR_LOG_LEVEL(DEBUG, "State machine transition: ("
                                  << Token << " , " << LToString(Source)
                                  << ") -> " << LToString(CurrentState));
      return CurrentState;
    }

    static EdgeFunction<l_t> compose(EdgeFunctionRef<TSEdgeFunction> This,
                                     const EdgeFunction<l_t> &SecondFunction) {
      if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
        return Default;
      }

      return TSEdgeFunctionComposer{This, SecondFunction, This->TSD};
    }

    static EdgeFunction<l_t> join(EdgeFunctionRef<TSEdgeFunction> This,
                                  const EdgeFunction<l_t> &OtherFunction) {
      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }

      return makeAllBottom(This->TSD);
    }

    bool operator==(const TSEdgeFunction &Other) const {
      return CallSite == Other.CallSite && Token == Other.Token;
    }

    friend llvm::raw_ostream &print(llvm::raw_ostream &OS,
                                    const TSEdgeFunction &TSE) {
      return OS << "TSEF(" << TSE.Token << " at "
                << llvmIRToShortString(TSE.CallSite) << ")";
    }
  };

  struct TSConstant : ConstantEdgeFunction<l_t> {
    std::conditional_t<HasJoinLatticeTraits<l_t>, EmptyType,
                       const TypeStateDescriptionTy *>
        TSD{};

    TSConstant(l_t Value, const TypeStateDescriptionTy *TSD) noexcept
        : ConstantEdgeFunction<l_t>{Value} {
      if constexpr (!HasJoinLatticeTraits<l_t>) {
        this->TSD = TSD;
      }
    }

    template <typename LL = l_t,
              typename = std::enable_if_t<HasJoinLatticeTraits<LL>>>
    TSConstant(l_t Value, EmptyType = {}) noexcept
        : ConstantEdgeFunction<l_t>{Value} {
      if constexpr (!HasJoinLatticeTraits<l_t>) {
        this->TSD = TSD;
      }
    }

    /// XXX: Cannot default compose() and join(), because l_t does not implement
    /// JoinLatticeTraits (because bottom value is not constant)
    template <typename ConcreteEF>
    static EdgeFunction<l_t> compose(EdgeFunctionRef<ConcreteEF> This,
                                     const EdgeFunction<l_t> &SecondFunction) {

      if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
        return Default;
      }

      l_t Ret = SecondFunction.computeTarget(This->Value);
      if (Ret == This->Value) {
        return This;
      }
      if (isBottom(Ret, This->TSD)) {
        return makeAllBottom(This->TSD);
      }

      return TSConstant{Ret, This->TSD};
    }

    template <typename ConcreteEF>
    static EdgeFunction<l_t> join(EdgeFunctionRef<ConcreteEF> This,
                                  const EdgeFunction<l_t> &OtherFunction) {
      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }

      auto Top = [TSD = This->TSD] {
        if constexpr (HasJoinLatticeTraits<l_t>) {
          return JoinLatticeTraits<l_t>::top();
        } else {
          return TSD->top();
        }
      }();
      if (const auto *C = llvm::dyn_cast<TSConstant>(OtherFunction)) {
        if (C->Value == This->Value || C->Value == Top) {
          return This;
        }
        if (This->Value == Top) {
          return OtherFunction;
        }
      }
      return makeAllBottom(This->TSD);
    }

    bool operator==(const TSConstant &Other) const noexcept {
      return this->Value == Other.Value;
    }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const TSConstant &EF) {
      return OS << "TSConstant[" << LToString(EF.Value) << "]";
    }
  };

public:
  IDETypeStateAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                       const TypeStateDescriptionTy *TSD,
                       std::vector<std::string> EntryPoints = {"main"})
      : IDETabProblemType(IRDB, std::move(EntryPoints), createZeroValue()),
        IDETypeStateAnalysisBase(PT,
                                 [TSD](llvm::StringRef Name) {
                                   return Name.contains(
                                       TSD->getTypeNameOfInterest());
                                 }),
        TSD(TSD) {
    assert(TSD != nullptr);
    assert(PT);
  }

  ~IDETypeStateAnalysis() override = default;

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override {
    // Check if Alloca's type matches the target type. If so, generate from zero
    // value.
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
      if (hasMatchingType(Alloca)) {
        return this->generateFromZero(Alloca);
      }
    }
    // Check load instructions for target type. Generate from the loaded value
    // and kill the load instruction if it was generated previously (strong
    // update!).
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      if (hasMatchingType(Load)) {
        return transferFlow(Load, Load->getPointerOperand());
      }
    }
    if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
      if (hasMatchingType(Gep->getPointerOperand())) {
        return identityFlow<d_t>();
        // return lambdaFlow<d_t>([=](d_t Source) -> std::set<d_t> {
        //   // if (Source == Gep->getPointerOperand()) {
        //   //  return {Source, Gep};
        //   //}
        //   return {Source};
        // });
      }
    }
    // Check store instructions for target type. Perform a strong update, i.e.
    // kill the alloca pointed to by the pointer-operand and all alloca's
    // related to the value-operand and then generate them from the
    // value-operand.
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      if (hasMatchingType(Store)) {
        auto RelevantAliasesAndAllocas = getLocalAliasesAndAllocas(
            Store->getPointerOperand(), // pointer- or value operand???
            // Store->getValueOperand(),
            Curr->getFunction()->getName().str());

        RelevantAliasesAndAllocas.insert(Store->getValueOperand());
        return lambdaFlow<d_t>(
            [Store, AliasesAndAllocas = std::move(RelevantAliasesAndAllocas)](
                d_t Source) -> container_type {
              // We kill all relevant loacal aliases and alloca's
              if (Source == Store->getPointerOperand()) {
                // XXX: later kill must-aliases too
                return {};
              }
              // Generate all local aliases and relevant alloca's from the
              // stored value
              if (Source == Store->getValueOperand()) {
                return AliasesAndAllocas;
              }
              return {Source};
            });
      }
    }
    return identityFlow<d_t>();
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override {
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

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override {

    /// TODO: Implement return-POI in LLVMFlowFunctions.h
    return lambdaFlow<d_t>([this, CalleeFun,
                            CS = llvm::cast<llvm::CallBase>(CallSite),
                            Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)](
                               d_t Source) -> container_type {
      if (LLVMZeroValue::isLLVMZeroValue(Source)) {
        return {Source};
      }
      container_type Res;
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
          for (unsigned Idx = CalleeFun->arg_size(); Idx < CS->arg_size();
               ++Idx) {
            Res.insert(CS->getArgOperand(Idx));
          }
        }
      }
      // Handle ordinary case
      // Map formal parameter into corresponding actual parameter.
      for (auto [Formal, Actual] : llvm::zip(CalleeFun->args(), CS->args())) {
        if (Source == &Formal) {
          Res.insert(Actual); // corresponding actual
        }
      }

      // Collect the return value
      if (Ret && Source == Ret->getReturnValue()) {
        Res.insert(CS);
      }

      // Collect all relevant alloca's to map into caller context
      {
        container_type RelAllocas;
        for (const auto *Fact : Res) {
          const auto &Allocas = getRelevantAllocas(Fact);
          RelAllocas.insert(Allocas.begin(), Allocas.end());
        }
        Res.insert(RelAllocas.begin(), RelAllocas.end());
      }

      return Res;
    });
  }

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    for (const auto *Callee : Callees) {
      std::string DemangledFname = llvm::demangle(Callee->getName().str());
      // Generate the return value of factory functions from zero value
      if (TSD->isFactoryFunction(DemangledFname)) {
        return this->generateFromZero(CS);
      }

      /// XXX: Revisit this:

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
    return identityFlow<d_t>();
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override {
    return nullptr;
  }

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return this->createDefaultSeeds();
  }

  [[nodiscard]] d_t createZeroValue() const {
    return LLVMZeroValue::getInstance();
  }

  [[nodiscard]] bool isZeroValue(d_t Fact) const override {
    return LLVMZeroValue::isLLVMZeroValue(Fact);
  }

  // in addition provide specifications for the IDE parts

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override {
    // Set alloca instructions of target type to uninitialized.
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
      if (hasMatchingType(Alloca)) {
        if (LLVMZeroValue::isLLVMZeroValue(CurrNode) && SuccNode == Alloca) {
          return TSConstant(TSD->uninit(), TSD);
        }
      }
    }
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction,
                                        d_t DestNode) override {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitInst, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    for (const auto *Callee : Callees) {
      std::string DemangledFname = llvm::demangle(Callee->getName().str());

      // For now we assume that we can only generate from the return value.
      // We apply the same edge function for the return value, i.e. callsite.
      if (TSD->isFactoryFunction(DemangledFname)) {
        PHASAR_LOG_LEVEL(DEBUG, "Processing factory function");
        if (isZeroValue(CallNode) && RetSiteNode == CS) {
          return TSConstant{
              TSD->getNextState(DemangledFname, TSD->uninit(), CS), TSD};
        }
      }

      // For every consuming parameter and all its aliases and relevant alloca's
      // we apply the same edge function.
      if (TSD->isConsumingFunction(DemangledFname)) {
        PHASAR_LOG_LEVEL(DEBUG, "Processing consuming function");
        for (auto Idx : TSD->getConsumerParamIdx(DemangledFname)) {
          const auto &AliasAndAllocas =
              getWMAliasesAndAllocas(CS->getArgOperand(Idx));

          if (CallNode == RetSiteNode && AliasAndAllocas.count(CallNode)) {
            return TSEdgeFunction{TSD, DemangledFname, CS};
          }
        }
      }
    }
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t CallSite, d_t CallNode,
                                           n_t RetSite,
                                           d_t RetSiteNode) override {
    return nullptr;
  }

  l_t topElement() override { return TSD->top(); }

  l_t bottomElement() override { return TSD->bottom(); }

  /**
   * We have a lattice with BOTTOM representing all information
   * and TOP representing no information. The other lattice elements
   * are defined by the type state description, i.e. represented by the
   * states of the finite state machine.
   *
   * @note Only one-level lattice's are handled currently
   */
  l_t join(l_t Lhs, l_t Rhs) override {
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

  EdgeFunction<l_t> allTopFunction() override {
    if constexpr (HasJoinLatticeTraits<l_t>) {
      return AllTop<l_t>{};
    } else {
      return AllTop<l_t>{topElement()};
    }
  }

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS = llvm::outs()) override {
    LLVMBasedCFG CFG;
    OS << "\n======= TYPE STATE RESULTS =======\n";
    for (const auto &F : this->IRDB->getAllFunctions()) {
      OS << '\n' << F->getName() << '\n';
      for (const auto &BB : *F) {
        for (const auto &I : BB) {
          auto Results = SR.resultsAt(&I, true);
          if (CFG.isExitInst(&I)) {
            OS << "\nAt exit stmt: " << NToString(&I) << '\n';
            for (auto Res : Results) {
              if (const auto *Alloca =
                      llvm::dyn_cast<llvm::AllocaInst>(Res.first)) {
                if (Res.second == TSD->error()) {
                  OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                     << DToString(Res.first) << '\n';
                  for (const auto *Pred : CFG.getPredsOf(&I)) {
                    OS << "\nPredecessor: " << NToString(Pred) << '\n';
                    auto PredResults = SR.resultsAt(Pred, true);
                    for (auto Res : PredResults) {
                      if (Res.first == Alloca) {
                        OS << "Pred State: " << LToString(Res.second) << '\n';
                      }
                    }
                  }
                  OS << "============================\n";
                } else {
                  OS << "\nAlloca : " << DToString(Res.first)
                     << "\nState  : " << LToString(Res.second) << '\n';
                }
              } else {
                OS << "\nInst: " << NToString(&I) << '\n'
                   << "Fact: " << DToString(Res.first) << '\n'
                   << "State: " << LToString(Res.second) << '\n';
              }
            }
          } else {
            for (auto Res : Results) {
              if (const auto *Alloca =
                      llvm::dyn_cast<llvm::AllocaInst>(Res.first)) {
                if (Res.second == TSD->error()) {
                  OS << "\n=== ERROR STATE DETECTED ===\nAlloca: "
                     << DToString(Res.first) << '\n'
                     << "\nAt IR Inst: " << NToString(&I) << '\n';
                  for (const auto *Pred : CFG.getPredsOf(&I)) {
                    OS << "\nPredecessor: " << NToString(Pred) << '\n';
                    auto PredResults = SR.resultsAt(Pred, true);
                    for (auto Res : PredResults) {
                      if (Res.first == Alloca) {
                        OS << "Pred State: " << LToString(Res.second) << '\n';
                      }
                    }
                  }
                  OS << "============================\n";
                }
              } else {
                OS << "\nInst: " << NToString(&I) << '\n'
                   << "Fact: " << DToString(Res.first) << '\n'
                   << "State: " << LToString(Res.second) << '\n';
              }
            }
          }
        }
      }
      OS << "\n--------------------------------------------\n";
    }
  }

private:
  const TypeStateDescriptionTy *TSD{};
};

template <typename TypeStateDescriptionTy>
IDETypeStateAnalysis(const LLVMProjectIRDB *, LLVMAliasInfoRef,
                     const TypeStateDescriptionTy *,
                     std::vector<std::string> EntryPoints)
    -> IDETypeStateAnalysis<TypeStateDescriptionTy>;

// class CSTDFILEIOTypeStateDescription;

// extern template class IDETypeStateAnalysis<CSTDFILEIOTypeStateDescription>;

} // namespace psr

#endif
