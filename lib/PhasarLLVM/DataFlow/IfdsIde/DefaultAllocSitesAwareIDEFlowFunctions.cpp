#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAllocSitesAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

using namespace psr;

using FFTemplates = FlowFunctionTemplates<
    detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::d_t,
    detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::FlowFunctionType::
        container_type>;
using container_type = FFTemplates::container_type;

static const llvm::Value *getBase(const llvm::Value *V,
                                  const llvm::DataLayout &DL) {
  // TODO: Optimize!

  llvm::APInt Offset(64, 0);
  const auto *Base = V->stripAndAccumulateConstantOffsets(DL, Offset, true);

  return Base->stripPointerCastsAndAliases();
}

static container_type
getReachableAllocationSites(const LLVMBasePointerAliasSet &AS,
                            const llvm::Value *Pointer,
                            const llvm::Instruction *Context) {
  if (!Pointer->getType()->isPointerTy()) {
    return {Pointer};
  }

  const auto &DL = Context->getModule()->getDataLayout();

  container_type Ret;
  auto AllocSites = AS.getAliasSet(Pointer, Context);
  for (const auto *Alias : *AllocSites) {
    const auto *AliasBase = getBase(Alias, DL);
    Ret.insert(AliasBase);
  }
  if (Ret.empty()) {
    Ret.insert(Pointer);
  }

  return Ret;
}

auto detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::
    getNormalFlowFunctionImpl(n_t Curr, n_t Succ) -> FlowFunctionPtrType {

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {

    const auto &DL = Store->getModule()->getDataLayout();
    const auto *PointerBase = getBase(Store->getPointerOperand(), DL);
    const auto *ValueBase = Store->getValueOperand();
    container_type Gen = getReachableAllocationSites(AS, PointerBase, Store);

    // llvm::errs() << "At store " << llvmIRToString(Curr)
    //              << ": ReachableAllocationSites: " << PrettyPrinter{Gen}
    //              << '\n';

    Gen.insert(ValueBase);
    // auto ValueAllocSites =
    //     getReachableAllocationSites(AS, Store->getValueOperand(), Store);

    if (EnableStrongUpdateStore) {

      return FFTemplates::lambdaFlow(
          [PointerBase, ValueBase,
           Gen{std::move(Gen)}](d_t Source) -> container_type {
            if (PointerBase == Source) {
              return {};
            }

            if (ValueBase == Source) {
              auto Ret = Gen;
              return Ret;
            }

            return {Source};
          });
    }

    return FFTemplates::lambdaFlow(
        [Gen{std::move(Gen)}, ValueBase](d_t Source) -> container_type {
          if (ValueBase == Source) {
            auto Ret = Gen;
            Ret.insert(Source);
            return Ret;
          }

          return {Source};
        });
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    const auto &DL = Load->getModule()->getDataLayout();
    const auto *PointerBase = getBase(Load->getPointerOperand(), DL);

    return FFTemplates::lambdaFlow(
        [PointerBase, Load](d_t Source) -> container_type {
          if (Source == PointerBase) {
            return {Source, Load};
          }

          return {Source};
        });
  }

  return this->IDENoAliasDefaultFlowFunctionsImpl::getNormalFlowFunctionImpl(
      Curr, Succ);
}

auto detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::
    getCallFlowFunctionImpl(n_t CallInst, f_t CalleeFun)
        -> FlowFunctionPtrType {
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(CallInst)) {
    return mapFactsToCallee(CallSite, CalleeFun);
  }

  return FFTemplates::killAllFlows();
}

static container_type getReturnedAliases(const container_type &Facts,
                                         const LLVMBasePointerAliasSet &AS,
                                         const llvm::Instruction *CallSite) {
  container_type Ret;
  for (const auto *Fact : Facts) {
    const auto &AllocSites = getReachableAllocationSites(AS, Fact, CallSite);
    Ret.insert(AllocSites.begin(), AllocSites.end());
  }

  return Ret;
}

auto detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::getRetFlowFunctionImpl(
    n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst, n_t /*RetSite*/)
    -> FlowFunctionPtrType {
  container_type Gen;

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    const auto PostProcessFacts = [AS = AS, Call](container_type &Facts) {
      Facts = getReturnedAliases(Facts, AS, Call);
    };

    const auto PropagateParameter = [](d_t Formal, d_t Source) {
      if (!Formal->getType()->isPointerTy()) {
        return false;
      }

      return Formal == Source;
    };

    const auto &DL = Call->getModule()->getDataLayout();

    const auto PropagateRet = [&DL](d_t RetVal, d_t Source) {
      return getBase(RetVal, DL) == Source;
    };

    return mapFactsToCaller(Call, ExitInst, PropagateParameter, PropagateRet,
                            {}, true, true, PostProcessFacts);
  }

  return FFTemplates::killAllFlows();
}

auto detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::
    getCallToRetFlowFunctionImpl(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees)
        -> FlowFunctionPtrType {
  return this->IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl(
      CallSite, RetSite, Callees);
}
