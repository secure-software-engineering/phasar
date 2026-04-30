#include "phasar/PhasarLLVM/DataFlow/MonoIfds/Problems/MonoIFDSTaintAnalysis.h"

#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/MapUtils.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

using namespace psr;
using namespace psr::monoifds;

using d_t = monoifds::TaintAnalysis::d_t;

static void insertOrAssign(DataFlowEnvironment<d_t> &Into, auto &&Key,
                           auto &&Value) {
  auto [It, Inserted] = Into.try_emplace(PSR_FWD(Key), PSR_FWD(Value));
  if (!Inserted) {
    It->second = PSR_FWD(Value);
  }
}

static void generateFlow(DataFlowEnvironment<d_t> &InOut, const llvm::Value *To,
                         const llvm::Value *From) {
  assert(To != From);

  // safety copy
  if (auto PtrSrc = getOrDefault(InOut, From); !PtrSrc.empty()) {
    // TODO: Strong update

    auto [It, Inserted] = InOut.try_emplace(To, std::move(PtrSrc));
    if (!Inserted) {
      It->second.insertAllOf(PtrSrc);
    }

    if (llvm::isa<llvm::Instruction>(From) && !From->hasNUsesOrMore(2)) {
      InOut.erase(From);
    }
  }
}

static void handleStore(DataFlowEnvironment<d_t> &InOut,
                        const llvm::StoreInst *Store, AliasCache &AC) {
  const auto *ValueOp = Store->getValueOperand();
  const auto *PointerOp = Store->getPointerOperand();

  if (auto ValueSrc = getOrDefault(InOut, ValueOp); !ValueSrc.empty()) {
    bool KillValue =
        llvm::isa<llvm::Instruction>(ValueOp) && !ValueOp->hasNUsesOrMore(2);

    auto Aliases = AC.getAliasSet(Store->getPointerOperand(), Store);

    for (const auto *Alias : Aliases) {
      if (Alias == PointerOp) {
        continue;
      }
      if (Alias == ValueOp) {
        KillValue = false;
        continue;
      }

      auto [It, Inserted] = InOut.try_emplace(Alias, ValueSrc);
      if (!Inserted) {
        It->second.insertAllOf(ValueSrc);
      }
    }

    insertOrAssign(InOut, PointerOp, std::move(ValueSrc));

    if (KillValue) {
      InOut.erase(ValueOp);
    }

  } else {
    InOut.erase(PointerOp);
  }
}

void monoifds::TaintAnalysis::LocalAnalysis::normalFlow(
    DataFlowEnvironment<d_t> &InOut, n_t Curr) {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return handleStore(InOut, Store, AC);
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return generateFlow(InOut, Load, Load->getPointerOperand());
  }

  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return generateFlow(InOut, GEP, GEP->getPointerOperand());
  }

  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Curr)) {
    return generateFlow(InOut, Cast, Cast->getOperand(0));
  }

  if (const auto *Extract = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {
    return generateFlow(InOut, Extract, Extract->getOperand(0));
  }

  for (const auto *Op : Curr->operand_values()) {
    if (Op->hasOneUser()) {
      InOut.erase(Op);
    }
  }

  // Otherwise we do not care and leave everything as it is
}

void monoifds::TaintAnalysis::LocalAnalysis::callToRetFlow(
    DataFlowEnvironment<d_t> &InOut, n_t Curr) {
  for (const auto &Arg : llvm::cast<llvm::CallBase>(Curr)->args()) {
    if (Arg->getType()->isPointerTy()) {
      InOut.erase(Arg);
    }
  }
}

auto monoifds::TaintAnalysis::LocalAnalysis::returnFlow(n_t CallSite, d_t Fact)
    -> llvm::SmallVector<d_t> {
  if (llvm::isa<llvm::Constant>(Fact)) {
    // Pass global variables as is, if desired
    // Globals could also be actual arguments, then the formal argument
    // needs to be generated below. Need llvm::Constant here to cover also
    // ConstantExpr and ConstantAggregate
    return {Fact};
  }

  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Fact)) {
    auto ArgNo = Arg->getArgNo();

    if (ArgNo >= Call->arg_size()) {
      llvm::report_fatal_error("Invalid Argument: Arg " + llvm::Twine(ArgNo) +
                               " at call with " +
                               llvm::Twine(Call->arg_size()) +
                               " arguments: " + psr::llvmIRToString(Call) +
                               " --- " + psr::llvmIRToString(Arg));
    }
    assert(ArgNo < Call->arg_size());

    const auto *ActualArg = Call->getArgOperand(ArgNo);

    if (llvm::isa<llvm::ConstantExpr>(ActualArg) ||
        llvm::isa<llvm::ConstantData>(ActualArg)) {
      return {};
    }

    return llvm::to_vector(AC.getAliasSet(ActualArg, Call));
  }

  if (Call->getFunctionType()->isVarArg()) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Fact);
        Alloca && isVaListAlloca(*Alloca)) {
      llvm::SmallVector<const llvm::Value *> Ret;

      auto NumParams = Call->getFunctionType()->getNumParams();
      for (const auto &Arg : llvm::drop_begin(Call->args(), NumParams)) {
        if (llvm::isa<llvm::ConstantExpr>(Arg.get()) ||
            llvm::isa<llvm::ConstantData>(Arg.get())) {
          continue;
        }
        if (Arg->getType()->isPointerTy()) {
          auto Aliases = AC.getAliasSet(Arg, Call);
          Ret.append(Aliases.begin(), Aliases.end());
        }
      }

      return Ret;
    }
  }

  // Everything else that has been found worthy to be mapped back must be a
  // return value
  return {CallSite};
}

auto monoifds::TaintAnalysis::LocalAnalysis::invCallFlow(n_t CallSite, d_t Fact)
    -> llvm::SmallVector<d_t> {
  if (llvm::isa<llvm::Constant>(Fact)) {
    // Pass global variables as is, if desired
    // Globals could also be actual arguments, then the formal argument
    // needs to be generated below. Need llvm::Constant here to cover also
    // ConstantExpr and ConstantAggregate
    return {Fact};
  }

  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Fact)) {
    auto ArgNo = Arg->getArgNo();

    if (ArgNo >= Call->arg_size()) {
      llvm::report_fatal_error("Invalid Argument: Arg " + llvm::Twine(ArgNo) +
                               " at call with " +
                               llvm::Twine(Call->arg_size()) +
                               " arguments: " + psr::llvmIRToString(Call) +
                               " --- " + psr::llvmIRToString(Arg));
    }
    assert(ArgNo < Call->arg_size());

    const auto *ActualArg = Call->getArgOperand(ArgNo);

    if (llvm::isa<llvm::ConstantExpr>(ActualArg) ||
        llvm::isa<llvm::ConstantData>(ActualArg)) {
      return {};
    }

    return {ActualArg};
  }

  if (Call->getFunctionType()->isVarArg()) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Fact);
        Alloca && isVaListAlloca(*Alloca)) {
      llvm::SmallVector<const llvm::Value *> Ret;

      auto NumParams = Call->getFunctionType()->getNumParams();
      for (const auto &Arg : llvm::drop_begin(Call->args(), NumParams)) {
        if (llvm::isa<llvm::ConstantExpr>(Arg.get()) ||
            llvm::isa<llvm::ConstantData>(Arg.get())) {
          continue;
        }
        if (Arg->getType()->isPointerTy()) {
          Ret.push_back(Arg);
        }
      }
      return Ret;
    }
  }

  // Everything else that has been found worthy to be mapped back must be a
  // return value
  return {CallSite};
}

void monoifds::TaintAnalysis::LocalAnalysis::initialSeeds(
    DataFlowEnvironment<d_t> &SeedState,
    Compressor<d_t, SourceFactId> &SeedCompressor, f_t Fun) {

  for (const auto &Arg : Fun->args()) {
    if (Arg.hasStructRetAttr() ||
        Arg.hasAttribute(llvm::Attribute::WriteOnly) ||
        !Arg.hasNUsesOrMore(1)) {
      continue;
    }

    if (TA->Config->skipSeed(&Arg)) {
      continue;
    }

    SeedState[&Arg].insert(SeedCompressor.getOrInsert(&Arg));
  }

  for (const auto *Glob : TA->UsedGlobals->GlobsPerSCC[CurrSCC]) {
    if (TA->Config->skipSeed(Glob)) {
      continue;
    }

    SeedState[Glob].insert(SeedCompressor.getOrInsert(Glob));
  }

  if (Fun->isVarArg()) {
    if (const auto *VA = getVaListTagOrNull(*Fun)) {
      SeedState[VA].insert(SeedCompressor.getOrInsert(VA));
    }
  }
}

void monoifds::TaintAnalysis::LocalAnalysis::generateFactsAtCall(
    n_t CS, f_t Callee, llvm::function_ref<void(d_t)> GenFact) {
  forallGeneratedFacts(*TA->Config, llvm::cast<llvm::CallBase>(CS), Callee,
                       [this, CS, GenFact](const auto *Fact) {
                         if (Fact->getType()->isPointerTy()) {
                           auto Aliases = AC.getAliasSet(Fact, CS);
                           llvm::for_each(Aliases, GenFact);
                         } else {
                           std::invoke(GenFact, Fact);
                         }
                       });
}

void monoifds::TaintAnalysis::LocalAnalysis::requestResultCallbackAtCallSite(
    n_t CS, f_t Callee, llvm::function_ref<void(d_t)> LeakFact) {
  forallLeakedFacts(*TA->Config, llvm::cast<llvm::CallBase>(CS), Callee,
                    LeakFact);
}

bool monoifds::TaintAnalysis::shouldBeInSummary(d_t ExitFact, n_t ExitInst) {
  if (llvm::isa<llvm::Constant>(ExitFact)) {
    // Global vars should be in summary
    return !llvm::isa<llvm::Function>(ExitFact);
  }

  const auto *RetStmt = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
  if (RetStmt && RetStmt->getReturnValue() == ExitFact) {
    // The return value should be in summary
    return true;
  }

  const auto *Fun = ExitInst->getFunction();

  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(ExitFact)) {
    if (Fun->isVarArg() && psr::isVaListAlloca(*Alloc)) {
      return true;
    }

    // Locals do not escape
    return false;
  }

  // Only output parameters can escape (i.e., pointer args)
  if (!ExitFact->getType()->isPointerTy()) {
    return false;
  }

  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(ExitFact)) {
    if (Arg->hasByValAttr()) {
      // This parameter is actually passed by value in the src code, just for
      // ABI reasons it appears as being passed by pointer
      return false;
    }

    return true;
  }

  return false;
}
