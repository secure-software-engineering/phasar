/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEINSTINTERACTIONALYSIS_H
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEINSTINTERACTIONALYSIS_H

#include "phasar/DataFlow/IfdsIde/DefaultEdgeFunctionSingletonCache.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMSolverResults.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// Have some handy helper functionalities

namespace vara {
class Taint;
} // namespace vara

namespace psr {

// [[nodiscard]] static inline const llvm::AllocaInst *
// getAllocaInstruction(const llvm::GetElementPtrInst *GEP) {
//   if (!GEP) {
//     return nullptr;
//   }
//   const auto *Alloca = GEP->getPointerOperand();
//   while (const auto *NestedGEP =
//              llvm::dyn_cast<llvm::GetElementPtrInst>(Alloca)) {
//     Alloca = NestedGEP->getPointerOperand();
//   }
//   return llvm::dyn_cast<llvm::AllocaInst>(Alloca);
// }

template <typename T> static BitVectorSet<T> bvSetFrom(const std::set<T> &Set) {
  BitVectorSet<T> Ret;
  Ret.reserve(Set.size());
  Ret.insert(Set.begin(), Set.end());
  return Ret;
}

template <typename Callable, typename... ArgTys>
static std::invoke_result_t<std::decay_t<Callable>, ArgTys...>
invoke_or_default(Callable &&Fn, ArgTys &&...Args) {
  if (!Fn) {
    return {};
  }

  return std::invoke(Fn, std::forward<ArgTys>(Args)...);
}

class IDEIIAFlowFact {

public:
  constexpr static unsigned KLimit = 2;

private:
  const llvm::Value *BaseVal = nullptr;
  llvm::SmallVector<const llvm::GetElementPtrInst *, KLimit> FieldDesc;

public:
  ~IDEIIAFlowFact() = default;
  IDEIIAFlowFact() = default;
  /// Constructs a data-flow fact from the base value. Fields are ignored. Use
  /// the factory function(s) to construct field-sensitive data-flow facts.
  IDEIIAFlowFact(const llvm::Value *BaseVal);
  /// Construct a data-flow fact from the base value and field specification.
  IDEIIAFlowFact(
      const llvm::Value *BaseVal,
      llvm::SmallVector<const llvm::GetElementPtrInst *, KLimit> FieldDesc);

  /// Creates a new data-flow fact of the base value that respects fields. Field
  /// accesses that exceed the specified depth will be collapsed and not further
  /// distinguished.
  static IDEIIAFlowFact create(const llvm::Value *BaseVal);

  /// Returns whether this data-flow fact describes the same abstract memory
  /// location than the other one.
  [[nodiscard]] bool flowFactEqual(const IDEIIAFlowFact &Other) const;

  [[nodiscard]] inline const llvm::Value *getBase() const { return BaseVal; }
  [[nodiscard]] inline llvm::SmallVector<const llvm::GetElementPtrInst *,
                                         KLimit>
  getField() const {
    return FieldDesc;
  }
  [[nodiscard]] inline constexpr unsigned getKLimit() const { return KLimit; }

  void print(llvm::raw_ostream &OS, bool IsForDebug = false) const;

  bool operator==(const IDEIIAFlowFact &Other) const;
  bool operator!=(const IDEIIAFlowFact &Other) const;
  bool operator<(const IDEIIAFlowFact &Other) const;

  bool operator==(const llvm::Value *V) const;

  inline operator const llvm::Value *() { return BaseVal; }

  [[nodiscard]] std::string str() const {
    std::string Ret;
    llvm::raw_string_ostream OS(Ret);
    print(OS);
    return Ret;
  }
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const IDEIIAFlowFact &FlowFact) {
  FlowFact.print(OS);
  return OS;
}

inline std::ostream &operator<<(std::ostream &OS,
                                const IDEIIAFlowFact &FlowFact) {
  llvm::raw_os_ostream Rso(OS);
  FlowFact.print(Rso);
  return OS;
}

} // namespace psr

// Implementations of STL traits.
namespace std {
template <> struct hash<psr::IDEIIAFlowFact> {
  size_t operator()(const psr::IDEIIAFlowFact &FlowFact) const {
    return std::hash<const llvm::Value *>()(FlowFact.getBase());
  }
};
} // namespace std

// Compatibility with LLVM Casting
namespace llvm {
template <> struct simplify_type<psr::IDEIIAFlowFact> {
  using SimpleType = const llvm::Value *;

  static SimpleType getSimplifiedValue(const psr::IDEIIAFlowFact &FF) noexcept {
    return FF.getBase();
  }
};
} // namespace llvm

namespace psr {

template <typename EdgeFactType>
struct IDEInstInteractionAnalysisDomain : public LLVMAnalysisDomainDefault {
  // type of the element contained in the sets of edge functions
  using d_t = IDEIIAFlowFact;
  using e_t = EdgeFactType;
  using l_t = LatticeDomain<BitVectorSet<e_t>>;
};

///
/// SyntacticAnalysisOnly: Can be set if a syntactic-only analysis is desired
/// (without using points-to information)
///
/// IndirectTaints: Can be set to ensure non-interference
///
template <typename EdgeFactType = std::string,
          bool SyntacticAnalysisOnly = false, bool EnableIndirectTaints = false>
class IDEInstInteractionAnalysisT
    : public IDETabulationProblem<
          IDEInstInteractionAnalysisDomain<EdgeFactType>> {
  using IDETabulationProblem<
      IDEInstInteractionAnalysisDomain<EdgeFactType>>::generateFromZero;

public:
  using AnalysisDomainTy = IDEInstInteractionAnalysisDomain<EdgeFactType>;

  using IDETabProblemType = IDETabulationProblem<AnalysisDomainTy>;
  using typename IDETabProblemType::container_type;
  using typename IDETabProblemType::FlowFunctionPtrType;

  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  // type of the element contained in the sets of edge functions
  using e_t = typename AnalysisDomainTy::e_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using EdgeFunctionType = EdgeFunction<l_t>;

  using EdgeFactGeneratorTy = std::set<e_t>(
      std::variant<n_t, const llvm::GlobalVariable *> InstOrGlobal);

  IDEInstInteractionAnalysisT(
      const LLVMProjectIRDB *IRDB, const LLVMBasedICFG *ICF,
      LLVMAliasInfoRef PT, std::vector<std::string> EntryPoints = {"main"},
      std::function<EdgeFactGeneratorTy> EdgeFactGenerator = nullptr)
      : IDETabulationProblem<AnalysisDomainTy, container_type>(
            IRDB, std::move(EntryPoints), createZeroValue()),
        ICF(ICF), PT(PT), EdgeFactGen(std::move(EdgeFactGenerator)) {
    assert(ICF != nullptr);
    assert(PT);
    // IIAAAddLabelsEF::initEdgeFunctionCleaner();
    // IIAAKillOrReplaceEF::initEdgeFunctionCleaner();
  }

  ~IDEInstInteractionAnalysisT() override = default;

  /// Offer a special hook to the user that allows to generate additional
  /// edge facts on-the-fly. Above the generator function, the ordinary
  /// edge facts are generated according to the usual edge functions.
  inline void registerEdgeFactGenerator(
      std::function<EdgeFactGeneratorTy> EdgeFactGenerator) {
    EdgeFactGen = std::move(EdgeFactGenerator);
  }

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t /* Succ */) override {
    // Generate all local variables
    //
    // Flow function:
    //
    //                0
    //                |\
    // x = alloca y   | \
    //                v  v
    //                0  x
    //
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
      PHASAR_LOG_LEVEL(DFADEBUG, "AllocaInst");
      return generateFromZero(Alloca);
    }

    // Handle indirect taints, i. e., propagate values that depend on branch
    // conditions whose operands are tainted.
    if constexpr (EnableIndirectTaints) {
      if (const auto *Br = llvm::dyn_cast<llvm::BranchInst>(Curr);
          Br && Br->isConditional()) {
        // If the branch is conditional and its condition is tainted, then we
        // need to propagates the instructions that are depending on this
        // branch, too.
        //
        // Flow function:
        //
        // Let I be the set of instructions of the branch instruction's
        // successors.
        //
        //                                          0  c  x
        //                                          |  |\
        // x = br C, label if.then, label if.else   |  | \--\
        //                                          v  v  v  v
        //                                          0  c  x  I
        //
        return lambdaFlow<d_t>([Br](d_t Src) {
          container_type Facts;
          Facts.insert(Src);
          if (Src == Br->getCondition()) {
            Facts.insert(Br);
            for (const auto *Succs : Br->successors()) {
              for (const auto &Inst : Succs->instructionsWithoutDebug()) {
                Facts.insert(&Inst);
              }
            }
          }
          return Facts;
        });
      }
    }

    // Handle points-to information if the user wishes to conduct a
    // non-syntax-only inst-interaction analysis.
    if constexpr (!SyntacticAnalysisOnly) {

      // (ii) Handle semantic propagation (pointers) for load instructions.

      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
        // If one of the potentially many loaded values holds, the load itself
        // (dereferenced value) must also be generated and populated.
        //
        // Flow function:
        //
        // Let Y = pts(y), be the points-to set of y.
        //
        //              0  Y  x
        //              |  |\
        // x = load y   |  | \
        //              v  v  v
        //              0  Y  x
        //
        return generateFlowIf<d_t>(
            Load, [PointerOp = Load->getPointerOperand(),
                   PTS = PT.getReachableAllocationSites(
                       Load->getPointerOperand(), OnlyConsiderLocalAliases)](
                      d_t Src) { return Src == PointerOp || PTS->count(Src); });
      }

      // (ii) Handle semantic propagation (pointers) for store instructions.
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
        // If the value to be stored holds, the potential memory location(s)
        // that it is stored to must be generated and populated, too.
        //
        // Flow function:
        //
        // Let X be
        //    - pts(x), the points-to set of x, if x is an intersting pointer.
        //    - a singleton set containing x, otherwise.
        //
        // Let Y be pts(y), the points-to set of y.
        //
        //             0  X  y
        //             |  |\ |
        // store x y   |  | \|
        //             v  v  v
        //             0  x  Y
        //
        return lambdaFlow<d_t>(
            [Store, PointerPTS = PT.getReachableAllocationSites(
                        Store->getPointerOperand(), OnlyConsiderLocalAliases,
                        Store)](d_t Src) -> container_type {
              if (Store->getPointerOperand() == Src || PointerPTS->count(Src)) {
                // Here, we are unsound!
                return {};
              }
              container_type Facts;
              Facts.insert(Src);
              // y/Y now obtains its new value(s) from x/X
              // If a value is stored that holds we must generate all potential
              // memory locations the store might write to.
              if (Store->getValueOperand() == Src) {
                Facts.insert(Store->getPointerOperand());
                Facts.insert(PointerPTS->begin(), PointerPTS->end());
              }
              // ... or from zero, if a constant literal is stored to y
              if (llvm::isa<llvm::ConstantData>(Store->getValueOperand()) &&
                  IDEInstInteractionAnalysisT::isZeroValueImpl(Src)) {
                Facts.insert(Store->getPointerOperand());
                Facts.insert(PointerPTS->begin(), PointerPTS->end());
              }
              return Facts;
            });
      }
    }

    // (i) Handle syntactic propagation

    // Handle load instruction
    //
    // Flow function:
    //
    //              0  y  x
    //              |  |\
    // x = load y   |  | \
    //              v  v  v
    //              0  y  x
    //
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      return generateFlow<d_t>(Load, Load->getPointerOperand());
    }
    // Handle store instructions
    //
    // Flow function:
    //
    //             0  x  y
    //             |  |\ |
    // store x y   |  | \|
    //             v  v  v
    //             0  x  y
    //
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      return lambdaFlow<d_t>([Store](d_t Src) -> container_type {
        // Override old value, i.e., kill value that is written to and
        // generate from value that is stored.
        if (Store->getPointerOperand() == Src) {
          return {};
        }
        container_type Facts = {Src};
        // y now obtains its new value from x
        if (Store->getValueOperand() == Src) {
          Facts.insert(Store->getPointerOperand());
        }
        // ... or from zero, if a constant literal is stored to y
        if (llvm::isa<llvm::ConstantData>(Store->getValueOperand()) &&
            IDEInstInteractionAnalysisT::isZeroValueImpl(Src)) {
          Facts.insert(Store->getPointerOperand());
        }
        IF_LOG_ENABLED({
          for (const auto Fact : Facts) {
            PHASAR_LOG_LEVEL(DFADEBUG, "Create edge: "
                                           << llvmIRToShortString(Src) << " --"
                                           << llvmIRToShortString(Store)
                                           << "--> " << Fact);
          }
        });
        return Facts;
      });
    }
    // At last, we can handle all other (unary/binary) instructions.
    //
    // Flow function:
    //
    //                       0  x  o  p
    //                       |  | /| /|
    // x = instruction o p   |  |/ |/ |
    //                       |  | /|  |
    //                       |  |/ |  |
    //                       v  v  v  v
    //                       0  x  o  p
    //
    return lambdaFlow<d_t>([Inst = Curr](d_t Src) {
      container_type Facts;
      if (IDEInstInteractionAnalysisT::isZeroValueImpl(Src)) {
        // keep the zero flow fact
        Facts.insert(Src);
        return Facts;
      }

      // continue syntactic propagation: populate and propagate other existing
      // facts
      for (auto &Op : Inst->operands()) {
        // if one of the operands holds, also generate the instruction using
        // it
        if (Op == Src) {
          Facts.insert(Inst);
        }
      }
      // pass everything that already holds as identity
      Facts.insert(Src);
      IF_LOG_ENABLED({
        for (const auto Fact : Facts) {
          PHASAR_LOG_LEVEL(DFADEBUG,
                           "Create edge: " << llvmIRToShortString(Src) << " --"
                                           << llvmIRToShortString(Inst)
                                           << "--> " << Fact);
        }
      });
      return Facts;
    });
  }

  inline FlowFunctionPtrType getCallFlowFunction(n_t CallSite,
                                                 f_t DestFun) override {
    if (this->ICF->isHeapAllocatingFunction(DestFun)) {
      // Kill add facts and model the effects in getCallToRetFlowFunction().
      return killAllFlows<d_t>();
    }
    if (DestFun->isDeclaration()) {
      // We don't have anything that we could analyze, kill all facts.
      return killAllFlows<d_t>();
    }
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

    // Map actual to formal parameters.
    auto MapFactsToCalleeFF = mapFactsToCallee<d_t>(
        CS, DestFun, [CS](const llvm::Value *ActualArg, ByConstRef<d_t> Src) {
          if (d_t(ActualArg) != Src) {
            return false;
          }

          if (CS->hasStructRetAttr() && ActualArg == CS->getArgOperand(0)) {
            return false;
          }

          return true;
        });

    // Generate the artificially introduced RVO parameters from zero value.
    auto SRetFormal = CS->hasStructRetAttr() ? DestFun->getArg(0) : nullptr;

    if (SRetFormal) {
      return unionFlows(
          std::move(MapFactsToCalleeFF),
          generateFlowAndKillAllOthers(SRetFormal, this->getZeroValue()));
    }

    return MapFactsToCalleeFF;
  }

  inline FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                                n_t ExitInst,
                                                n_t /* RetSite */) override {
    // Map return value back to the caller. If pointer parameters hold at the
    // end of a callee function generate all of those in the caller context.

    auto MapFactsToCallerFF =
        mapFactsToCaller<d_t>(llvm::cast<llvm::CallBase>(CallSite), ExitInst,
                              {}, [](const llvm::Value *RetVal, d_t Src) {
                                if (Src == RetVal) {
                                  return true;
                                }
                                if (isZeroValueImpl(Src)) {
                                  if (llvm::isa<llvm::ConstantData>(RetVal)) {
                                    return true;
                                  }
                                }
                                return false;
                              });

    return MapFactsToCallerFF;
  }

  inline FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t /* RetSite */,
                           llvm::ArrayRef<f_t> Callees) override {
    // Model call to heap allocating functions (new, new[], malloc, etc.) --
    // only model direct calls, though.
    if (Callees.size() == 1) {
      const auto *Callee = Callees.front();
      if (this->ICF->isHeapAllocatingFunction(Callee)) {
        // In case a heap allocating function is called, generate the pointer
        // that is returned.
        //
        // Flow function:
        //
        // Let H be a heap allocating function.
        //
        //              0
        //              |\
        // x = call H   | \
        //              v  v
        //              0  x
        //
        return generateFromZero(CallSite);
      }
    }
    // Just use the auto mapping for values; pointer parameters and global
    // variables are killed and handled by getCallFlowfunction() and
    // getRetFlowFunction().
    // However, if only declarations are available as callee targets we would
    // lose the data-flow facts involved in the call which is usually not the
    // behavior that is intended. In that case, we must propagate all data-flow
    // facts alongside the call site.
    bool OnlyDecls = true;
    bool AllVoidRetTys = true;
    for (auto Callee : Callees) {
      if (!Callee->isDeclaration()) {
        OnlyDecls = false;
      }
      if (!Callee->getReturnType()->isVoidTy()) {
        AllVoidRetTys = false;
      }
    }

    return lambdaFlow<d_t>([CallSite = llvm::cast<llvm::CallBase>(CallSite),
                            OnlyDecls,
                            AllVoidRetTys](d_t Source) -> container_type {
      // There are a few things to consider, in case only declarations of
      // callee targets are available.
      if (OnlyDecls) {
        if (!AllVoidRetTys) {
          // If one or more of the declaration-only targets return a value, it
          // must be generated from zero!
          if (isZeroValueImpl(Source)) {
            return {Source, CallSite};
          }
        }
        // If all declaration-only callee targets return void, just pass
        // everything as identity.
        return {Source};
      }
      // Do not pass global variables if definitions of the callee
      // function(s) are available, since the effect of the callee on these
      // values will be modelled using combined getCallFlowFunction and
      // getReturnFlowFunction.
      if (llvm::isa<llvm::Constant>(Source.getBase())) {
        return {};
      }
      // Pass everything else as identity. In particular, also do not kill
      // pointer or reference parameters since this then also captures usages
      // oft the parameters, which we wish to compute using this analysis.
      return {Source};
    });
  }

  inline FlowFunctionPtrType
  getSummaryFlowFunction(n_t /* CallSite */, f_t /* DestFun */) override {
    // Do not use user-crafted summaries.
    return nullptr;
  }

  inline InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    InitialSeeds<n_t, d_t, l_t> Seeds;

    forallStartingPoints(this->EntryPoints, ICF, [this, &Seeds](n_t SP) {
      // Set initial seeds at the required entry points and generate the global
      // variables using generalized initial seeds

      // Generate zero value at the entry points
      Seeds.addSeed(SP, this->getZeroValue(), bottomElement());
      // Generate formal parameters of entry points, e.g. main(). Formal
      // parameters will otherwise cause trouble by overriding alloca
      // instructions without being valid data-flow facts themselves.
      for (const auto &Arg : SP->getFunction()->args()) {
        Seeds.addSeed(SP, &Arg, Bottom{});
      }
      // Generate all global variables using generalized initial seeds

      for (const auto &G : this->IRDB->getModule()->globals()) {
        if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
          l_t InitialValues = BitVectorSet<e_t>();
          std::set<e_t> EdgeFacts;
          if (EdgeFactGen) {
            EdgeFacts = EdgeFactGen(GV);
            // fill BitVectorSet
            InitialValues =
                BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
          }
          Seeds.addSeed(SP, GV, InitialValues);
        }
      }
    });

    return Seeds;
  }

  [[nodiscard]] inline d_t createZeroValue() const {
    // Create a special value to represent the zero value!
    return LLVMZeroValue::getInstance();
  }

  inline bool isZeroValue(d_t d) const override { return isZeroValueImpl(d); }

  // In addition provide specifications for the IDE parts.

  inline EdgeFunctionType getStrongUpdateStoreEF(const llvm::StoreInst *Store,
                                                 d_t CurrNode, d_t SuccNode,
                                                 l_t UserEdgeFacts) {

    // Overriding edge: obtain labels from value to be stored (and may add
    // UserEdgeFacts, if any).
    //
    // x --> y
    //
    // Edge function:
    //
    //            x
    //             \
    // store x y    \ \x.x \cup { commit of('store x y') }
    //               v
    //               y
    //

    if (CurrNode == Store->getValueOperand() ||
        (isZeroValue(CurrNode) &&
         llvm::isa<llvm::Constant>(Store->getValueOperand()))) {
      if (SuccNode == Store->getPointerOperand() ||
          PT.isInReachableAllocationSites(Store->getPointerOperand(), SuccNode,
                                          true, Store)) {
        return IIAAAddLabelsEFCache.createEdgeFunction(UserEdgeFacts);
      }
    }

    if (SyntacticAnalysisOnly) {
      // Kill all labels that are propagated along the edge of the value that
      // is overridden.
      //
      // y --> y
      //
      // Edge function:
      //
      //               y
      //               |
      // store x y     | \x.{ commit of('store x y') }
      //               v
      //               y
      //
      if ((CurrNode == SuccNode) && CurrNode == Store->getPointerOperand()) {
        // y obtains its value(s) from its original allocation and the store
        // instruction under analysis.
        IF_LOG_ENABLED({
          PHASAR_LOG_LEVEL(DFADEBUG,
                           "Const-Replace at '" << llvmIRToString(Store));
          PHASAR_LOG_LEVEL(DFADEBUG, "Replacement label(s): ");
          for (const auto &Item : UserEdgeFacts.assertGetValue()) {
            PHASAR_LOG_LEVEL(DFADEBUG, Item << ", ");
          }
          PHASAR_LOG_LEVEL(DFADEBUG, '\n');
        });
        // obtain label from the original allocation
        return IIAAKillOrReplaceEFCache.createEdgeFunction(UserEdgeFacts);
      }

    } else {
      // Use points-to information to find all possible overriding edges.

      // Kill all labels that are propagated along the edge of the
      // value/values that is/are overridden.
      //
      // y --> y
      //
      // Edge function:
      //
      //            y
      //            |
      // store x y  | \x.{}
      //            v
      //            y
      //
      if (CurrNode == SuccNode && (Store->getPointerOperand() == CurrNode ||
                                   this->PT.isInReachableAllocationSites(
                                       Store->getPointerOperand(), CurrNode,
                                       OnlyConsiderLocalAliases))) {
        return IIAAKillOrReplaceEFCache.createEdgeFunction(BitVectorSet<e_t>());
      }
    }

    if (CurrNode == SuccNode) {
      // Everything unrelated to the store
      return EdgeIdentity<l_t>{};
    }

    llvm::report_fatal_error(
        llvm::Twine("Unhandled edge: \n> Store: ") + llvmIRToString(Store) +
        "\n> CurrNode: " + CurrNode.str() + "\n> SuccNode: " + SuccNode.str());
  }

  inline EdgeFunctionType getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                n_t /* Succ */,
                                                d_t SuccNode) override {
    PHASAR_LOG_LEVEL(DFADEBUG,
                     "Process edge: " << llvmIRToShortString(CurrNode) << " --"
                                      << llvmIRToString(Curr) << "--> "
                                      << llvmIRToShortString(SuccNode));

    if (isZeroValue(SuccNode)) {
      // We don't want to propagate any facts on zero
      return EdgeIdentity<l_t>{};
    }

    // Overrides at store instructions
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      return getStrongUpdateStoreEF(
          Store, CurrNode, SuccNode,
          bvSetFrom(invoke_or_default(EdgeFactGen, Curr)));
    }

    //
    // Identity edges
    //
    // Edge function:
    //
    //                     0
    //                     |
    // %i = instruction    | \x.x
    //                     v
    //                     0
    //
    if (CurrNode == SuccNode) {
      return EdgeIdentity<l_t>{};
    }

    if (Curr != CurrNode && Curr == SuccNode) {
      // check if the user has registered a fact generator function
      l_t UserEdgeFacts = bvSetFrom(invoke_or_default(EdgeFactGen, Curr));

      // We generate Curr in this instruction, so we have to annotate it with
      // edge labels
      return IIAAAddLabelsEFCache.createEdgeFunction(UserEdgeFacts);
    }

    // Otherwise stick to identity.
    return EdgeIdentity<l_t>{};
  }

  inline EdgeFunctionType getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                              f_t /* DestinationMethod */,
                                              d_t DestNode) override {
    // Handle the case in which a parameter that has been artificially
    // introduced by the compiler is passed. Such a value must be generated from
    // the zero value, to reflact the fact that the data flows from the callee
    // to the caller (at least) according to the source code.
    //
    // Let a_i be an argument that is annotated by the sret attribute.
    //
    // 0 --> a_i
    //
    // Edge function:
    //
    //                      0
    //                       \
    // call/invoke f(a_i)     \ \x.{}
    //                         v
    //                         a_i
    //
    std::set<d_t> SRetParams;
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      if (CS->paramHasAttr(Idx, llvm::Attribute::StructRet)) {
        SRetParams.insert(CS->getArgOperand(Idx));
      }
    }
    if (isZeroValue(SrcNode) && SRetParams.count(DestNode)) {
      return IIAAAddLabelsEFCache.createEdgeFunction();
    }
    // Everything else can be passed as identity.
    return EdgeIdentity<l_t>{};
  }

  inline EdgeFunctionType
  getReturnEdgeFunction(n_t CallSite, f_t /* CalleeMethod */, n_t ExitInst,
                        d_t ExitNode, n_t /* RetSite */, d_t RetNode) override {
    // Handle the case in which constant data is returned, e.g. ret i32 42.
    //
    // Let c be the return instruction's corresponding call site.
    //
    // 0 --> c
    //
    // Edge function:
    //
    //               0
    //                \
    // ret x           \ \x.x \cup { commit of('ret x') }
    //                  v
    //                  c
    //
    if (isZeroValue(ExitNode) && RetNode == CallSite) {
      const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
      if (const auto *CD =
              llvm::dyn_cast<llvm::ConstantData>(Ret->getReturnValue())) {
        // Check if the user has registered a fact generator function
        l_t UserEdgeFacts = BitVectorSet<e_t>();
        std::set<e_t> EdgeFacts;
        if (EdgeFactGen) {
          EdgeFacts = EdgeFactGen(ExitInst);
          // fill BitVectorSet
          UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
        }
        return IIAAAddLabelsEFCache.createEdgeFunction(
            std::move(UserEdgeFacts));
      }
    }
    // Everything else can be passed as identity.
    return EdgeIdentity<l_t>{};
  }

  inline EdgeFunctionType
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t /* RetSite */,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override {
    // Check if the user has registered a fact generator function
    l_t UserEdgeFacts = BitVectorSet<e_t>();
    std::set<e_t> EdgeFacts;
    if (EdgeFactGen) {
      EdgeFacts = EdgeFactGen(CallSite);
      // fill BitVectorSet
      UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
    }
    // Model call to heap allocating functions (new, new[], malloc, etc.) --
    // only model direct calls, though.
    if (Callees.size() == 1) {
      for (const auto *Callee : Callees) {
        if (this->ICF->isHeapAllocatingFunction(Callee)) {
          // Let H be a heap allocating function.
          //
          // 0 --> x
          //
          // Edge function:
          //
          //               0
          //                \
          // %i = call H     \ \x.x \cup { commit of('%i = call H') }
          //                  v
          //                  i
          //
          if (isZeroValue(CallNode) && RetSiteNode == CallSite) {
            return IIAAAddLabelsEFCache.createEdgeFunction(
                std::move(UserEdgeFacts));
          }
        }
      }
    }
    // Capture interactions of the call instruction and its arguments.
    const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite);
    for (const auto &Arg : CS->args()) {
      //
      // o_i --> o_i
      //
      // Edge function:
      //
      //                 o_i
      //                 |
      // %i = call o_i   | \ \x.x \cup { commit of('%i = call H') }
      //                 v
      //                 o_i
      //
      if (CallNode == Arg && CallNode == RetSiteNode) {
        return IIAAAddLabelsEFCache.createEdgeFunction(
            std::move(UserEdgeFacts));
      }
    }
    // Otherwise stick to identity
    return EdgeIdentity<l_t>{};
  }

  inline EdgeFunctionType
  getSummaryEdgeFunction(n_t /* CallSite */, d_t /* CallNode */,
                         n_t /* RetSite */, d_t /* RetSiteNode */) override {
    // Do not use user-crafted summaries.
    return nullptr;
  }

  inline l_t topElement() override { return Top{}; }

  inline l_t bottomElement() override { return Bottom{}; }

  inline l_t join(l_t Lhs, l_t Rhs) override { return joinImpl(Lhs, Rhs); }

  inline EdgeFunctionType allTopFunction() override { return AllTop<l_t>(); }

  // Provide some handy helper edge functions to improve reuse.

  // Edge function that kills all labels in a set (and may replaces them with
  // others).
  struct IIAAKillOrReplaceEF {
    using l_t = typename AnalysisDomainTy::l_t;
    l_t Replacement{};

    l_t computeTarget(ByConstRef<l_t> /* Src */) const { return Replacement; }

    static EdgeFunction<l_t> compose(EdgeFunctionRef<IIAAKillOrReplaceEF> This,
                                     const EdgeFunction<l_t> SecondFunction) {

      if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
        return Default;
      }

      auto Cache = This.getCacheOrNull();
      assert(Cache != nullptr && "We expect a cache, because "
                                 "IIAAKillOrReplaceEF is too large for SOO");

      if (auto *AD = llvm::dyn_cast<IIAAAddLabelsEF>(SecondFunction)) {
        auto ADCache =
            SecondFunction.template getCacheOrNull<IIAAAddLabelsEF>();
        assert(ADCache != nullptr);
        if (This->isKillAll()) {
          return ADCache->createEdgeFunction(*AD);
        }
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(This->Replacement, AD->Data);
        return ADCache->createEdgeFunction(std::move(Union));
      }

      if (auto *KR = llvm::dyn_cast<IIAAKillOrReplaceEF>(SecondFunction)) {
        if (This->isKillAll()) {
          return Cache->createEdgeFunction(*KR);
        }
        if (KR->isKillAll()) {
          return SecondFunction;
        }
        auto Union = IDEInstInteractionAnalysisT::joinImpl(This->Replacement,
                                                           KR->Replacement);
        return Cache->createEdgeFunction(std::move(Union));
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAKillOrReplaceEF'");
    }

    static EdgeFunction<l_t> join(EdgeFunctionRef<IIAAKillOrReplaceEF> This,
                                  const EdgeFunction<l_t> &OtherFunction) {
      /// XXX: Here, we underapproximate joins with EdgeIdentity
      if (llvm::isa<EdgeIdentity<l_t>>(OtherFunction)) {
        return This;
      }

      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }

      auto Cache = This.getCacheOrNull();
      assert(Cache != nullptr && "We expect a cache, because "
                                 "IIAAKillOrReplaceEF is too large for SOO");

      if (auto *AD = llvm::dyn_cast<IIAAAddLabelsEF>(OtherFunction)) {
        auto ADCache = OtherFunction.template getCacheOrNull<IIAAAddLabelsEF>();
        assert(ADCache);
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(This->Replacement, AD->Data);
        return ADCache->createEdgeFunction(std::move(Union));
      }
      if (auto *KR = llvm::dyn_cast<IIAAKillOrReplaceEF>(OtherFunction)) {
        auto Union = IDEInstInteractionAnalysisT::joinImpl(This->Replacement,
                                                           KR->Replacement);
        return Cache->createEdgeFunction(std::move(Union));
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAKillOrReplaceEF'");
    }

    bool operator==(const IIAAKillOrReplaceEF &Other) const noexcept {
      return Replacement == Other.Replacement;
    }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const IIAAKillOrReplaceEF &EF) {
      OS << "EF: (IIAAKillOrReplaceEF)<->";
      if (EF.isKillAll()) {
        OS << "(KillAll";
      } else {
        IDEInstInteractionAnalysisT::printEdgeFactImpl(OS, EF.Replacement);
      }
      return OS << ")";
    }

    [[nodiscard]] bool isKillAll() const noexcept {
      if (auto *RSet = std::get_if<BitVectorSet<e_t>>(&Replacement)) {
        return RSet->empty();
      }
      return false;
    }

    // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
    friend llvm::hash_code hash_value(const IIAAKillOrReplaceEF &EF) {
      return hash_value(EF.Replacement);
    }
  };

  // Edge function that adds the given labels to existing labels
  // add all labels provided by Data.
  struct IIAAAddLabelsEF {
    using l_t = typename AnalysisDomainTy::l_t;
    l_t Data{};

    l_t computeTarget(ByConstRef<l_t> Src) const {
      return IDEInstInteractionAnalysisT::joinImpl(Src, Data);
    }

    static EdgeFunction<l_t> compose(EdgeFunctionRef<IIAAAddLabelsEF> This,
                                     const EdgeFunction<l_t> &SecondFunction) {
      if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
        return Default;
      }

      auto Cache = This.getCacheOrNull();
      assert(Cache != nullptr && "We expect a cache, because "
                                 "IIAAAddLabelsEF is too large for SOO");

      if (auto *AD = llvm::dyn_cast<IIAAAddLabelsEF>(SecondFunction)) {
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(This->Data, AD->Data);
        return Cache->createEdgeFunction(std::move(Union));
      }
      if (auto *KR = llvm::dyn_cast<IIAAKillOrReplaceEF>(SecondFunction)) {
        return Cache->createEdgeFunction(KR->Replacement);
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAAddLabelsEF'");
    }

    static EdgeFunction<l_t> join(EdgeFunctionRef<IIAAAddLabelsEF> This,
                                  const EdgeFunction<l_t> &OtherFunction) {
      /// XXX: Here, we underapproximate joins with EdgeIdentity
      if (llvm::isa<EdgeIdentity<l_t>>(OtherFunction)) {
        return This;
      }

      if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
        return Default;
      }

      auto Cache = This.getCacheOrNull();
      assert(Cache != nullptr && "We expect a cache, because "
                                 "IIAAAddLabelsEF is too large for SOO");

      if (auto *AD = llvm::dyn_cast<IIAAAddLabelsEF>(OtherFunction)) {
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(This->Data, AD->Data);
        return Cache->createEdgeFunction(std::move(Union));
      }
      if (auto *KR = llvm::dyn_cast<IIAAKillOrReplaceEF>(OtherFunction)) {
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(This->Data, KR->Replacement);
        return Cache->createEdgeFunction(std::move(Union));
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAAddLabelsEF'");
    }

    bool operator==(const IIAAAddLabelsEF &Other) const noexcept {
      return Data == Other.Data;
    }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const IIAAAddLabelsEF &EF) {
      OS << "EF: (IIAAAddLabelsEF: ";
      IDEInstInteractionAnalysisT::printEdgeFactImpl(OS, EF.Data);
      return OS << ")";
    }

    // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
    friend llvm::hash_code hash_value(const IIAAAddLabelsEF &EF) {
      return hash_value(EF.Data);
    }
  };

  // Provide functionalities for printing things and emitting text reports.

  void printNode(llvm::raw_ostream &OS, n_t n) const override {
    OS << llvmIRToString(n);
  }

  void printDataFlowFact(llvm::raw_ostream &OS, d_t FlowFact) const override {
    OS << llvmIRToString(FlowFact);
  }

  void printFunction(llvm::raw_ostream &OS, f_t Fun) const override {
    OS << Fun->getName();
  }

  inline void printEdgeFact(llvm::raw_ostream &OS,
                            l_t EdgeFact) const override {
    printEdgeFactImpl(OS, EdgeFact);
  }

  static void stripBottomResults(std::unordered_map<d_t, l_t> &Res) {
    for (auto It = Res.begin(); It != Res.end();) {
      if (It->second.isBottom()) {
        It = Res.erase(It);
      } else {
        ++It;
      }
    }
  }

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS = llvm::outs()) override {
    OS << "\n====================== IDE-Inst-Interaction-Analysis Report "
          "======================\n";
    // if (!IRDB->debugInfoAvailable()) {
    //   // Emit only IR code, function name and module info
    //   OS << "\nWARNING: No Debug Info available - emiting results without "
    //         "source code mapping!\n";
    for (const auto *f : this->ICF->getAllFunctions()) {
      std::string FunName = getFunctionNameFromIR(f);
      OS << "\nFunction: " << FunName << "\n----------"
         << std::string(FunName.size(), '-') << '\n';
      for (const auto *Inst : this->ICF->getAllInstructionsOf(f)) {
        auto Results = SR.resultsAt(Inst, true);
        stripBottomResults(Results);
        if (!Results.empty()) {
          OS << "At IR statement: " << this->NtoString(Inst) << '\n';
          for (auto Result : Results) {
            if (!Result.second.isBottom()) {
              OS << "   Fact: " << this->DtoString(Result.first)
                 << "\n  Value: " << this->LtoString(Result.second) << '\n';
            }
          }
          OS << '\n';
        }
      }
      OS << '\n';
    }
    // } else {
    // TODO: implement better report in case debug information are available
    //   }
  }

  /// Computes all variables where a result set has been computed using the
  /// edge functions (and respective value domain).
  inline std::unordered_set<d_t>
  getAllVariables(const SolverResults<n_t, d_t, l_t> & /* Solution */) const {
    std::unordered_set<d_t> Variables;
    // collect all variables that are available
    const llvm::Module *M = this->IRDB->getModule();
    for (const auto &G : M->globals()) {
      Variables.insert(&G);
    }
    for (const auto *I : this->IRDB->getAllInstructions()) {
      if (const auto *A = llvm::dyn_cast<llvm::AllocaInst>(I)) {
        Variables.insert(A);
      }
      if (const auto *H = llvm::dyn_cast<llvm::CallBase>(I)) {
        if (!H->isIndirectCall() && H->getCalledFunction() &&
            this->ICF->isHeapAllocatingFunction(H->getCalledFunction())) {
          Variables.insert(H);
        }
      }
    }

    return Variables;
  }

  /// Computes all variables for which an empty set has been computed using the
  /// edge functions (and respective value domain).
  inline std::unordered_set<d_t> getAllVariablesWithEmptySetValue(
      const SolverResults<n_t, d_t, l_t> &Solution) const {
    return removeVariablesWithoutEmptySetValue(Solution,
                                               getAllVariables(Solution));
  }

protected:
  static inline bool isZeroValueImpl(d_t d) {
    return LLVMZeroValue::isLLVMZeroValue(d);
  }

  static void printEdgeFactImpl(llvm::raw_ostream &OS,
                                ByConstRef<l_t> EdgeFact) {
    if (std::holds_alternative<Top>(EdgeFact)) {
      OS << std::get<Top>(EdgeFact);
    } else if (std::holds_alternative<Bottom>(EdgeFact)) {
      OS << std::get<Bottom>(EdgeFact);
    } else {
      auto LSet = std::get<BitVectorSet<e_t>>(EdgeFact);
      OS << "(set size: " << LSet.size() << ") values: ";
      if constexpr (std::is_same_v<e_t, vara::Taint *>) {
        for (const auto &LElem : LSet) {
          std::string IRBuffer;
          llvm::raw_string_ostream RSO(IRBuffer);
          LElem->print(RSO);
          RSO.flush();
          OS << IRBuffer << ", ";
        }
      } else {
        for (const auto &LElem : LSet) {
          OS << LElem << ", ";
        }
      }
    }
  }

  static inline l_t joinImpl(ByConstRef<l_t> Lhs, ByConstRef<l_t> Rhs) {
    if (Lhs.isTop() || Lhs.isBottom()) {
      return Rhs;
    }
    if (Rhs.isTop() || Rhs.isBottom()) {
      return Lhs;
    }
    const auto &LhsSet = std::get<BitVectorSet<e_t>>(Lhs);
    const auto &RhsSet = std::get<BitVectorSet<e_t>>(Rhs);
    return LhsSet.setUnion(RhsSet);
  }

private:
  /// Filters out all variables that had a non-empty set during edge functions
  /// computations.
  inline std::unordered_set<d_t> removeVariablesWithoutEmptySetValue(
      const SolverResults<n_t, d_t, l_t> &Solution,
      std::unordered_set<d_t> Variables) const {
    // Check the solver results and remove all variables for which a
    // non-empty set has been computed
    auto Results = Solution.getAllResultEntries();
    for (const auto &Result : Results) {
      // We do not care for the concrete instruction at which data-flow facts
      // hold, instead we just wish to find out if a variable has been generated
      // at some point. Therefore, we only care for the variables and their
      // associated values and ignore at which point a variable may holds as a
      // data-flow fact.
      const auto Variable = Result.getColumnKey();
      const auto &Value = Result.getValue();
      // skip result entry if variable is not in the set of all variables
      if (Variables.find(Variable) == Variables.end()) {
        continue;
      }
      // skip result entry if the computed value is not of type BitVectorSet
      if (!std::holds_alternative<BitVectorSet<e_t>>(Value)) {
        continue;
      }
      // remove variable from result set if a non-empty that has been computed
      auto &Values = std::get<BitVectorSet<e_t>>(Value);
      if (!Values.empty()) {
        Variables.erase(Variable);
      }
    }
    return Variables;
  }

  DefaultEdgeFunctionSingletonCache<IIAAAddLabelsEF> IIAAAddLabelsEFCache;
  DefaultEdgeFunctionSingletonCache<IIAAKillOrReplaceEF>
      IIAAKillOrReplaceEFCache;

  const LLVMBasedICFG *ICF{};
  LLVMAliasInfoRef PT{};
  std::function<EdgeFactGeneratorTy> EdgeFactGen;
  static inline const bool OnlyConsiderLocalAliases = true;

}; // namespace psr

using IDEInstInteractionAnalysis = IDEInstInteractionAnalysisT<>;

} // namespace psr

#endif
