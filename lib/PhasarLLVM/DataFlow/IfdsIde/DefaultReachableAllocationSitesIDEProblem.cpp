#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultReachableAllocationSitesIDEProblem.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <cstdlib>

using namespace psr;

using FFTemplates = FlowFunctionTemplates<
    detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::d_t,
    detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
        FlowFunctionType::container_type>;
using container_type = FFTemplates::container_type;

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getNormalFlowFunctionImpl(n_t Curr, n_t Succ) -> FlowFunctionPtrType {

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    container_type Gen;

    auto AliasSet =
        AS.getReachableAllocationSites(Store->getPointerOperand(), true, Store);
    Gen.insert(AliasSet->begin(), AliasSet->end());
    Gen.insert(Store->getValueOperand());

    return FFTemplates::lambdaFlow(
        [Store, Gen{std::move(Gen)}, AS = AS](d_t Source) -> container_type {
          if (Store->getPointerOperand() == Source) {
            return {};
          }
          if (Store->getValueOperand() == Source ||
              AS.isInReachableAllocationSites(Store->getValueOperand(), Source,
                                              true, Store)) {
            return Gen;
          }

          return {Source};
        });
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    container_type Gen;

    auto AliasSet = AS.getReachableAllocationSites(Load, true, Load);
    Gen.insert(AliasSet->begin(), AliasSet->end());
    Gen.insert(Load);

    return FFTemplates::lambdaFlow(
        [Load, Gen{std::move(Gen)}, AS = AS](d_t Source) -> container_type {
          if (Load->getPointerOperand() == Source ||
              AS.isInReachableAllocationSites(Load->getPointerOperand(), Source,
                                              true, Load)) {
            return Gen;
          }

          return {Source};
        });
  }

  return this->IDENoAliasDefaultFlowFunctionsImpl::getNormalFlowFunctionImpl(
      Curr, Succ);
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getCallFlowFunctionImpl(n_t CallInst, f_t CalleeFun)
        -> FlowFunctionPtrType {
  const auto *Call = llvm::cast<llvm::CallBase>(CallInst);

  std::vector<LLVMAliasInfoRef::AllocationSiteSetPtrTy> AliasArgs;
  AliasArgs.reserve(Call->arg_size());

  for (const auto &CurrArg : Call->args()) {
    AliasArgs.emplace_back(AS.getReachableAllocationSites(CurrArg, true, Call));
  }

  return mapFactsToCallee(
      Call, CalleeFun,
      [Call, AliasArgs = std::move(AliasArgs)](d_t Arg, d_t Source) -> bool {
        if (Arg == Source) {
          return true;
        }

        if (Arg->getType()->isPointerTy()) {
          for (const auto &CurrArg : Call->args()) {
            if (Arg == CurrArg) {
              return AliasArgs[CurrArg.getOperandNo()]->contains(Source);
            }
          }
        }

        return false;
      });
}

static void populateWithMayAliases(LLVMAliasInfoRef AS, container_type &Facts,
                                   const llvm::Instruction *Context) {
  container_type Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = AS.getReachableAllocationSites(Fact, true, Context);
    for (const auto *Alias : *Aliases) {
      if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Alias)) {
        if (Inst->getParent() == Context->getParent() &&
            Context->comesBefore(Inst)) {
          // We will see that inst later
          continue;
        }
      }

      Tmp.insert(Alias);
    }
  }

  Facts = std::move(Tmp);
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getRetFlowFunctionImpl(n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst,
                           n_t /*RetSite*/) -> FlowFunctionPtrType {
  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  const auto PostProcessFacts = [AS = AS, Call](container_type &Facts) {
    populateWithMayAliases(AS, Facts, Call);
  };

  return mapFactsToCaller(
      Call, ExitInst,
      [AS = AS](d_t Param, d_t Source) {
        return Param->getType()->isPointerTy() &&
               (Param == Source ||
                AS.isInReachableAllocationSites(Param, Source));
      },
      [AS = AS](d_t Ret, d_t Source) {
        return Ret == Source || AS.isInReachableAllocationSites(Ret, Source);
      },
      {}, true, true, PostProcessFacts);
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getCallToRetFlowFunctionImpl(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees)
        -> FlowFunctionPtrType {
  return this->IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl(
      CallSite, RetSite, Callees);
}
