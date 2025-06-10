#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAllocSitesAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <iterator>

using namespace psr;

using FFTemplates = FlowFunctionTemplates<
    detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::d_t,
    detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::FlowFunctionType::
        container_type>;
using container_type = FFTemplates::container_type;

static container_type
getReachableAllocationSites(LLVMAliasInfoRef AS, const llvm::Value *Pointer,
                            const llvm::Instruction *Context) {
  if (!Pointer->getType()->isPointerTy()) {
    return {Pointer};
  }

  container_type Ret;
  auto AllocSites = AS.getReachableAllocationSites(Pointer, true, Context);
  Ret.insert(AllocSites->begin(), AllocSites->end());
  if (Ret.empty()) {
    Ret.insert(Pointer);
  }

  return Ret;
}

auto detail::IDEAllocSitesAwareDefaultFlowFunctionsImpl::
    getNormalFlowFunctionImpl(n_t Curr, n_t Succ) -> FlowFunctionPtrType {

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {

    container_type Gen =
        getReachableAllocationSites(AS, Store->getPointerOperand(), Store);

    auto ValueAllocSites =
        getReachableAllocationSites(AS, Store->getValueOperand(), Store);

    if (EnableStrongUpdateStore) {

      return FFTemplates::lambdaFlow([Store, Gen{std::move(Gen)},
                                      ValueAliases{std::move(ValueAllocSites)}](
                                         d_t Source) -> container_type {
        if (Store->getPointerOperand() == Source ||
            Store->getPointerOperand()->stripPointerCastsAndAliases() ==
                Source) {
          return {};
        }

        if (Store->getValueOperand() == Source || ValueAliases.count(Source)) {
          auto Ret = Gen;
          Ret.insert(Source);
          return Ret;
        }

        return {Source};
      });
    }

    return FFTemplates::lambdaFlow([Store, Gen{std::move(Gen)},
                                    ValueAliases{std::move(ValueAllocSites)}](
                                       d_t Source) -> container_type {
      if (Store->getValueOperand() == Source || ValueAliases.count(Source)) {
        auto Ret = Gen;
        Ret.insert(Source);
        return Ret;
      }

      return {Source};
    });
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    auto AllocSites =
        getReachableAllocationSites(AS, Load->getPointerOperand(), Load);

    return FFTemplates::lambdaFlow([Load, AllocSites{std::move(AllocSites)}](
                                       d_t Source) -> container_type {
      if (Source == Load->getPointerOperand() || AllocSites.count(Source)) {
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
    return mapFactsToCallee(
        CallSite, CalleeFun, [CallSite, AS = AS](d_t Arg, d_t Source) {
          if (Arg == Source) {
            return true;
          }

          return Arg->getType()->isPointerTy() &&
                 Source->getType()->isPointerTy() &&
                 AS.isInReachableAllocationSites(Arg, Source, true, CallSite);
        });
  }

  return FFTemplates::killAllFlows();
}

static container_type getReturnedAliases(const container_type &Facts,
                                         psr::LLVMAliasInfoRef AS,
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

    const auto PropagateParameter = [AS = AS, ExitInst](d_t Formal,
                                                        d_t Source) {
      if (!Formal->getType()->isPointerTy()) {
        return false;
      }

      return Formal == Source ||
             AS.isInReachableAllocationSites(Formal, Source, true, ExitInst);
    };

    const auto PropagateRet = [AS = AS, ExitInst](d_t RetVal, d_t Source) {
      if (RetVal == Source) {
        return true;
      }

      return RetVal->getType()->isPointerTy() &&
             Source->getType()->isPointerTy() &&
             AS.isInReachableAllocationSites(RetVal, Source, true, ExitInst);
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
