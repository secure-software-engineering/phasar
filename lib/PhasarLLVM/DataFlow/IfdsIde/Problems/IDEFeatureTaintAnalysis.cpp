#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEFeatureTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>
#include <tuple>

using namespace psr;

using l_t = IDEFeatureTaintAnalysisDomain::l_t;
using d_t = IDEFeatureTaintAnalysisDomain::d_t;

IDEFeatureTaintAnalysis::IDEFeatureTaintAnalysis(
    const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
    std::vector<std::string> EntryPoints, FeatureTaintGenerator &&TaintGen)
    : IDETabulationProblem<IDEFeatureTaintAnalysisDomain>(
          IRDB, std::move(EntryPoints), LLVMZeroValue::getInstance()),
      TaintGen(std::move(TaintGen)), PT(PT) {}

IDEFeatureTaintAnalysis::~IDEFeatureTaintAnalysis() = default;

std::string psr::LToString(const IDEFeatureTaintEdgeFact &EdgeFact) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);
  ROS << '<';
  llvm::interleaveComma(EdgeFact.Taints.set_bits(), ROS);
  ROS << '>';
  return Ret;
}

auto IDEFeatureTaintAnalysis::getNormalFlowFunction(n_t Curr, n_t /* Succ */)
    -> FlowFunctionPtrType {
  bool GeneratesFact = TaintGen.isSource(Curr);

  llvm::errs() << "[getNormalFlowFunction]: " << llvmIRToString(Curr) << '\n';

  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (GeneratesFact) {
      return generateFromZero(Alloca);
    }
    return identityFlow();
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return lambdaFlow(
        [GeneratesFact, Load, PointerOp = Load->getPointerOperand(),
         PTS = PT.getReachableAllocationSites(Load->getPointerOperand(), true)](
            d_t Source) -> container_type {
          bool GenFromZero =
              GeneratesFact && LLVMZeroValue::isLLVMZeroValue(Source);

          if (GenFromZero || Source == PointerOp || PTS->count(Source)) {
            return {Source, Load};
          }

          return {Source};
        });
  }

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return lambdaFlow([Store,
                       PointerPTS = PT.getReachableAllocationSites(
                           Store->getPointerOperand(), true, Store),
                       GeneratesFact](d_t Src) -> container_type {
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
      // ... or from zero, if we manually generate a fact here
      if (GeneratesFact && LLVMZeroValue::isLLVMZeroValue(Src)) {
        Facts.insert(Store->getPointerOperand());
        Facts.insert(PointerPTS->begin(), PointerPTS->end());
      }
      return Facts;
    });
  }

  // Fallback
  return lambdaFlow([Inst = Curr, GeneratesFact](d_t Src) {
    container_type Facts;
    Facts.insert(Src);
    if (LLVMZeroValue::isLLVMZeroValue(Src)) {
      if (GeneratesFact) {
        Facts.insert(Inst);
      }
      return Facts;
    }

    // continue syntactic propagation: populate and propagate other existing
    // facts
    for (const auto &Op : Inst->operands()) {
      // if one of the operands holds, also generate the instruction using
      // it
      if (Op == Src) {
        Facts.insert(Inst);
      }
    }
    return Facts;
  });
}

auto IDEFeatureTaintAnalysis::getCallFlowFunction(n_t CallSite, f_t DestFun)
    -> FlowFunctionPtrType {

  if (DestFun->isDeclaration()) {
    // We don't have anything that we could analyze, kill all facts.
    return killAllFlows();
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
  const auto *SRetFormal =
      CS->hasStructRetAttr() ? DestFun->getArg(0) : nullptr;

  if (SRetFormal && TaintGen.isSource(CallSite)) {
    return unionFlows(
        std::move(MapFactsToCalleeFF),
        generateFlowAndKillAllOthers(SRetFormal, this->getZeroValue()));
  }

  return MapFactsToCalleeFF;
}

auto IDEFeatureTaintAnalysis::getRetFlowFunction(n_t CallSite,
                                                 f_t /*CalleeFun*/,
                                                 n_t ExitInst,
                                                 n_t /* RetSite */)
    -> FlowFunctionPtrType {
  // Map return value back to the caller. If pointer parameters hold at the
  // end of a callee function generate all of those in the caller context.
  if (CallSite == nullptr) {
    return killAllFlows();
  }

  const auto *RetInst = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
  auto *RetVal = RetInst ? RetInst->getReturnValue() : nullptr;
  bool GeneratesFact = llvm::isa_and_nonnull<llvm::ConstantData>(RetVal) &&
                       TaintGen.isSource(ExitInst);
  return mapFactsToCaller<d_t>(
      llvm::cast<llvm::CallBase>(CallSite), ExitInst, {},
      [GeneratesFact](const llvm::Value *RetVal, d_t Src) {
        // ASK: Should all parameters be mapped back? Or just pointer params?
        // ASK: IDEInstInteractionAnalysisTest.HandleCallTest_01: Should 14
        // (%call) hold for 'k' at ret?
        if (Src == RetVal) {
          return true;
        }
        if (GeneratesFact && LLVMZeroValue::isLLVMZeroValue(Src)) {
          return true;
        }
        return false;
      });
}

auto IDEFeatureTaintAnalysis::getCallToRetFlowFunction(
    n_t CallSite, n_t /* RetSite */, llvm::ArrayRef<f_t> Callees)
    -> FlowFunctionPtrType {

  if (llvm::all_of(Callees, [](f_t Fun) { return Fun->isDeclaration(); })) {
    bool GeneratesFact =
        !CallSite->getType()->isVoidTy() && TaintGen.isSource(CallSite);
    if (GeneratesFact) {
      return generateFromZero(CallSite);
    }
    return identityFlow();
  }

  auto Mapper = mapFactsAlongsideCallSite(
      llvm::cast<llvm::CallBase>(CallSite),
      [](d_t Arg) {
        return !Arg->getType()->isPointerTy();
        // return llvm::isa<llvm::Constant>(Arg);
      },
      /*PropagateGlobals*/ false);

  // if (GeneratesFact) {
  //   return unionFlows(std::move(Mapper),
  //                     generateFlowAndKillAllOthers(CallSite,
  //                     getZeroValue()));
  // }
  return Mapper;
}

struct IDEFeatureTaintAnalysis::AddFactsEF {
  using l_t = IDEFeatureTaintAnalysisDomain::l_t;

  IDEFeatureTaintEdgeFact Facts;

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    Source.Taints |= Facts.Taints;
    return Source;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<AddFactsEF> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    llvm::report_fatal_error("Implemented in 'extend'");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<AddFactsEF> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Implemented in 'combine'");
  }

  friend bool operator==(const AddFactsEF &L, const AddFactsEF &R) {
    return L.Facts == R.Facts;
  }

  // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
  friend llvm::hash_code hash_value(const AddFactsEF &EF) {
    return hash_value(EF.Facts);
  }
};

struct IDEFeatureTaintAnalysis::GenerateEF {
  using l_t = IDEFeatureTaintAnalysisDomain::l_t;

  IDEFeatureTaintEdgeFact Facts;

  [[nodiscard]] bool isConstant() const noexcept { return true; }

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> /*Source*/) const {
    return Facts;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<GenerateEF> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    llvm::report_fatal_error("Implemented in 'extend'");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<GenerateEF> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Implemented in 'combine'");
  }

  friend bool operator==(const GenerateEF &L, const GenerateEF &R) {
    return L.Facts == R.Facts;
  }

  // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
  friend llvm::hash_code hash_value(const GenerateEF &EF) {
    return hash_value(EF.Facts);
  }
};

namespace {
struct AddSmallFactsEF {
  using l_t = IDEFeatureTaintAnalysisDomain::l_t;

  uintptr_t Facts{};

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    Source.unionWith(Facts);
    return Source;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<AddSmallFactsEF> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    llvm::report_fatal_error("Implemented in 'extend'");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<AddSmallFactsEF> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Implemented in 'combine'");
  }

  friend bool operator==(const AddSmallFactsEF &L, const AddSmallFactsEF &R) {
    return L.Facts == R.Facts;
  }

  // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
  friend llvm::hash_code hash_value(const AddSmallFactsEF &EF) {
    return llvm::hash_value(EF.Facts);
  }
};

struct GenerateSmallEF {
  using l_t = IDEFeatureTaintAnalysisDomain::l_t;

  uintptr_t Facts{};

  [[nodiscard]] bool isConstant() const noexcept { return true; }

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> /*Source*/) const {
    return Facts;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<GenerateSmallEF> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    llvm::report_fatal_error("Implemented in 'extend'");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<GenerateSmallEF> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Implemented in 'combine'");
  }

  // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
  friend llvm::hash_code hash_value(const GenerateSmallEF &EF) {
    return llvm::hash_value(EF.Facts);
  }

  friend bool operator==(GenerateSmallEF L, GenerateSmallEF R) {
    return L.Facts == R.Facts;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       GenerateSmallEF EF) {
    return OS << "GenerateSmallEF" << LToString(EF.computeTarget(0));
  }
};

// auto GenerateSmallEF::compose(EdgeFunctionRef<GenerateSmallEF> This,
//                               const EdgeFunction<l_t> &SecondFunction)
//     -> EdgeFunction<l_t> {
//   if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
//     return Default;
//   }

//   auto Val = SecondFunction.computeTarget(This->computeTarget(0));

//   if (Val.Taints.isSmall()) {
//     uintptr_t Buf{};
//     std::ignore = Val.Taints.getData(Buf);
//     return GenerateSmallEF{Buf};
//   }

//   // TODO: Caching

//   return GenerateEF{std::move(Val)};
// }

// auto AddSmallFactsEF::compose(EdgeFunctionRef<AddSmallFactsEF> This,
//                               const EdgeFunction<l_t> &SecondFunction)
//     -> EdgeFunction<l_t> {
//   if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
//     return Default;
//   }

//   auto Val = SecondFunction.computeTarget(This->computeTarget(0));

//   if (Val.Taints.isSmall()) {
//     uintptr_t Buf{};
//     std::ignore = Val.Taints.getData(Buf);
//     return AddSmallFactsEF{Buf};
//   }

//   // TODO: Caching

//   return AddFactsEF{std::move(Val)};
// }

// auto GenerateEF::compose(EdgeFunctionRef<GenerateEF> This,
//                          const EdgeFunction<l_t> &SecondFunction)
//     -> EdgeFunction<l_t> {
//   if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
//     return Default;
//   }

//   auto Val = SecondFunction.computeTarget(This->computeTarget(0));

//   // TODO: Caching

//   return GenerateEF{std::move(Val)};
// }

// auto AddFactsEF::compose(EdgeFunctionRef<AddFactsEF> This,
//                          const EdgeFunction<l_t> &SecondFunction)
//     -> EdgeFunction<l_t> {
//   if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
//     return Default;
//   }

//   auto Val = SecondFunction.computeTarget(This->computeTarget(0));

//   // TODO: Caching

//   return AddFactsEF{std::move(Val)};
// }

// template <typename GenEFTy>
// EdgeFunction<l_t> joinWithGen(EdgeFunctionRef<GenEFTy> This,
//                               const EdgeFunction<l_t> &OtherFunction) {
//   if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
//     return Default;
//   }

//   auto OtherFacts = OtherFunction.computeTarget(0);
//   OtherFacts.unionWith(This->Facts);

//   if (OtherFacts.Taints.isSmall()) {
//     uintptr_t Buf{};
//     std::ignore = OtherFacts.Taints.getData(Buf);

//     if (OtherFunction.isConstant()) {
//       return GenerateSmallEF{Buf};
//     }

//     return AddSmallFactsEF{Buf};
//   }

//   // TODO: Caching

//   if (OtherFunction.isConstant()) {
//     return GenerateEF{std::move(OtherFacts)};
//   }

//   return AddFactsEF{std::move(OtherFacts)};
// }

// template <typename AddEFTy>
// EdgeFunction<l_t> joinWithAdd(EdgeFunctionRef<AddEFTy> This,
//                               const EdgeFunction<l_t> &OtherFunction) {
//   /// XXX: Here, we underapproximate joins with EdgeIdentity
//   if (llvm::isa<EdgeIdentity<l_t>>(OtherFunction)) {
//     return This;
//   }

//   if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
//     return Default;
//   }

//   auto OtherFacts = OtherFunction.computeTarget(0);
//   OtherFacts.unionWith(This->Facts);

//   if (OtherFacts.Taints.isSmall()) {
//     uintptr_t Buf{};
//     std::ignore = OtherFacts.Taints.getData(Buf);

//     return AddSmallFactsEF{Buf};
//   }

//   // TODO: Caching

//   return AddFactsEF{std::move(OtherFacts)};
// }

// auto GenerateSmallEF::join(EdgeFunctionRef<GenerateSmallEF> This,
//                            const EdgeFunction<l_t> &OtherFunction)
//     -> EdgeFunction<l_t> {
//   return joinWithGen(This, OtherFunction);
// }
// auto GenerateEF::join(EdgeFunctionRef<GenerateEF> This,
//                       const EdgeFunction<l_t> &OtherFunction)
//     -> EdgeFunction<l_t> {
//   return joinWithGen(This, OtherFunction);
// }

// auto AddSmallFactsEF::join(EdgeFunctionRef<AddSmallFactsEF> This,
//                            const EdgeFunction<l_t> &OtherFunction)
//     -> EdgeFunction<l_t> {
//   return joinWithAdd(This, OtherFunction);
// }

// auto AddFactsEF::join(EdgeFunctionRef<AddFactsEF> This,
//                       const EdgeFunction<l_t> &OtherFunction)
//     -> EdgeFunction<l_t> {
//   return joinWithAdd(This, OtherFunction);
// }

///

template <typename CacheT>
EdgeFunction<l_t> genEF(l_t &&Facts, CacheT &GenEFCache) {
  if (Facts.Taints.isSmall()) {
    uintptr_t Buf{};
    std::ignore = Facts.Taints.getData(Buf);
    return GenerateSmallEF{Buf};
  }
  return GenEFCache.createEdgeFunction(std::move(Facts));
}

template <typename CacheT>
EdgeFunction<l_t> addEF(l_t &&Facts, CacheT &AddEFCache) {
  if (Facts.Taints.isSmall()) {
    uintptr_t Buf{};
    std::ignore = Facts.Taints.getData(Buf);
    return AddSmallFactsEF{Buf};
  }
  return AddEFCache.createEdgeFunction(std::move(Facts));
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

EdgeFunction<l_t>
IDEFeatureTaintAnalysis::extend(const EdgeFunction<l_t> &FirstEF,
                                const EdgeFunction<l_t> &SecondEF) {
  auto Ret = [&] {
    if (auto Default = defaultComposeOrNull(FirstEF, SecondEF)) {
      llvm::errs() << "defaultComposeOrNull>>\n";
      return Default;
    }

    auto Val = SecondEF.computeTarget(FirstEF.computeTarget(0));

    if (FirstEF.isConstant()) {
      return genEF(std::move(Val), GenEFCache);
    }

    return addEF(std::move(Val), AddEFCache);
  }();

  llvm::errs() << "Extend " << FirstEF << " with " << SecondEF << " --> " << Ret
               << '\n';

  return Ret;
}
EdgeFunction<l_t>
IDEFeatureTaintAnalysis::combine(const EdgeFunction<l_t> &FirstEF,
                                 const EdgeFunction<l_t> &OtherEF) {
  auto Ret = [&] {
    /// XXX: Here, we underapproximate joins with EdgeIdentity
    if (llvm::isa<EdgeIdentity<l_t>>(FirstEF)) {
      return OtherEF;
    }
    if (llvm::isa<EdgeIdentity<l_t>>(OtherEF) &&
        !llvm::isa<AllTop<l_t>>(FirstEF)) {
      return FirstEF;
    }

    if (auto Default = defaultJoinOrNull(FirstEF, OtherEF)) {
      return Default;
    }

    auto ThisFacts = FirstEF.computeTarget(0);
    ThisFacts.unionWith(OtherEF.computeTarget(0));

    if (FirstEF.isConstant() && OtherEF.isConstant()) {
      return genEF(std::move(ThisFacts), GenEFCache);
    }

    return addEF(std::move(ThisFacts), AddEFCache);
  }();

  llvm::errs() << "Combine " << FirstEF << " and " << OtherEF << " --> " << Ret
               << '\n';
  return Ret;
}

auto IDEFeatureTaintAnalysis::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                    n_t /* Succ */,
                                                    d_t SuccNode)
    -> EdgeFunction<l_t> {

  if (isZeroValue(SuccNode) || CurrNode == SuccNode) {
    // We don't want to propagate any facts on zero
    llvm::errs() << "Identity Edge\n";
    return EdgeIdentity<l_t>{};
  }

  if (isZeroValue(CurrNode)) {
    llvm::errs() << "Generate from Zero\n";
    // Generate user edge-facts from zero
    return genEF(TaintGen.getGeneratedTaintsAt(Curr), GenEFCache);
  }

  // Overrides at store instructions
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    if (CurrNode == Store->getValueOperand()) {
      // Store tainted value

      // propagate facts unchanged. User edge-facts are generated from zero.

      llvm::errs() << "Store Identity\n";
      return EdgeIdentity<l_t>{};
    }
  }

  llvm::errs() << "Fallback Identity\n";
  // Otherwise stick to identity.
  return EdgeIdentity<l_t>{};
}

auto IDEFeatureTaintAnalysis::getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                                  f_t /*DestinationFunction*/,
                                                  d_t DestNode)
    -> EdgeFunction<l_t> {
  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    // Generate user edge-facts from zero
    return genEF(TaintGen.getGeneratedTaintsAt(CallSite), GenEFCache);
  }

  return EdgeIdentity<l_t>{};
}

auto IDEFeatureTaintAnalysis::getReturnEdgeFunction(
    n_t CallSite, f_t /*CalleeFunction*/, n_t ExitStmt, d_t ExitNode,
    n_t /*RetSite*/, d_t RetNode) -> EdgeFunction<l_t> {
  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    // Generate user edge-facts from zero
    return genEF(TaintGen.getGeneratedTaintsAt(ExitStmt), GenEFCache);
  }

  return EdgeIdentity<l_t>{};
}

auto IDEFeatureTaintAnalysis::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t /*RetSite*/, d_t RetSiteNode,
    llvm::ArrayRef<f_t> /*Callees*/) -> EdgeFunction<l_t> {
  if (isZeroValue(CallNode) && !isZeroValue(RetSiteNode)) {
    // Generate user edge-facts from zero

    llvm::errs() << "At CTR " << llvmIRToString(CallSite)
                 << ": Gen from zero!\n";
    return genEF(TaintGen.getGeneratedTaintsAt(CallSite), GenEFCache);
  }
  return EdgeIdentity<l_t>{};
}

auto IDEFeatureTaintAnalysis::initialSeeds() -> InitialSeeds<n_t, d_t, l_t> {
  InitialSeeds<n_t, d_t, l_t> Seeds;

  LLVMBasedCFG CFG;
  forallStartingPoints(this->EntryPoints, IRDB, CFG, [this, &Seeds](n_t SP) {
    // Set initial seeds at the required entry points and generate the global
    // variables using generalized initial seeds

    // Generate zero value at the entry points
    Seeds.addSeed(SP, this->getZeroValue(), 0);
    // Generate formal parameters of entry points, e.g. main(). Formal
    // parameters will otherwise cause trouble by overriding alloca
    // instructions without being valid data-flow facts themselves.

    /// TODO: Do we want that? --NO
    // for (const auto &Arg : SP->getFunction()->args()) {
    //   Seeds.addSeed(SP, &Arg, BitVectorSet<e_t>());
    // }
    // Generate all global variables using generalized initial seeds

    for (const auto &G : this->IRDB->getModule()->globals()) {
      if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(&G)) {
        l_t InitialValues = TaintGen.getGeneratedTaintsAt(GV);
        if (InitialValues.Taints.any()) {
          Seeds.addSeed(SP, GV, std::move(InitialValues));
        }
      }
    }
  });

  Seeds.dump(llvm::errs());

  return Seeds;
}

bool IDEFeatureTaintAnalysis::isZeroValue(d_t FlowFact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(FlowFact);
}

void IDEFeatureTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, llvm::raw_ostream &OS) {
  OS << "\n====================== IDE-Inst-Interaction-Analysis Report "
        "======================\n";

  for (const auto *F : IRDB->getAllFunctions()) {
    auto FunName = F->getName();
    OS << "\nFunction: " << FunName << "\n----------"
       << std::string(FunName.size(), '-') << '\n';

    for (const auto &Inst : llvm::instructions(F)) {
      auto Results = SR.resultsAt(&Inst, true);
      // stripBottomResults(Results);
      if (!Results.empty()) {
        OS << "At IR statement: " << NToString(Inst) << '\n';
        for (const auto &Result : Results) {
          if (!Result.second.isBottom()) {
            OS << "   Fact: " << DToString(Result.first)
               << "\n  Value: " << TaintGen.toString(Result.second) << '\n';
          }
        }
        OS << '\n';
      }
    }
    OS << '\n';
  }
}
