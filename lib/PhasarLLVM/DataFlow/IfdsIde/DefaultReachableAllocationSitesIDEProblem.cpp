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

    auto AliasSet =
        AS.getReachableAllocationSites(Load->getPointerOperand(), Load);
    Gen.insert(AliasSet->begin(), AliasSet->end());
    // Gen.insert(Load->getValueOperand());

    return FFTemplates::lambdaFlow(
        [Load, Gen{std::move(Gen)}, AS = AS](d_t Source) -> container_type {
          // TODO: Fabian fragen, ob getPointerOperand() hier richtig ist.
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

  // TODO: das hab ich mir noch aufgeschrieben, passt aber nicht zu der impl
  // hier drunter.

  // mapfactstocallee
  // PropagateArgumentWithSource
  // das brauchen wir bei dem call

  // TODO: Fabian fragen, ob diese Impl so passt.
  const auto *Call = llvm::cast<llvm::CallBase>(CallInst);

  std::vector<LLVMAliasInfoRef::AllocationSiteSetPtrTy> AliasArgs(
      Call->arg_size());

  for (const auto &CurrArg : Call->args()) {
    AliasArgs.emplace_back(AS.getReachableAllocationSites(CurrArg, true, Call));
  }

  if (!AliasArgs.empty()) {
    return FFTemplates::lambdaFlow(
        [AliasArgs{std::move(AliasArgs)}](d_t Source) -> container_type {
          container_type Gen;

          for (const auto &CurrArg : AliasArgs) {
            if (CurrArg->contains(Source)) {
              Gen.insert(CurrArg->begin(), CurrArg->end());
            }
          }

          return Gen;
        });
  }

  return mapFactsToCallee(Call, CalleeFun, [](d_t Arg, d_t Source) -> bool {
    return Arg == Source;
  });
}

static void populateWithMayAliases(LLVMAliasInfoRef AS, container_type &Facts,
                                   const llvm::Instruction *Context) {
  container_type Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = AS.getReachableAllocationSites(Fact, Context);
    for (const auto *Alias : *Aliases) {
      if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Alias)) {
        if (Inst->getParent() == Context->getParent() &&
            Context->comesBefore(Inst)) {
          // We will see that inst later
          continue;
        }
      }

      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
        // Handle at least one level of indirection...
        const auto *PointerOp = Load->getPointerOperand()->stripPointerCasts();
        Tmp.insert(PointerOp);
      }

      Tmp.insert(Alias);
    }
  }

  Facts = std::move(Tmp);
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getRetFlowFunctionImpl(n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst,
                           n_t /*RetSite*/) -> FlowFunctionPtrType {
  // TODO:
  // ähnlich zu dem return bei dem aliasreturn problem
  // betrifft nur die reachableallocationsites
  // prüfen, ob source in den reachableallocationsites drin ist

  // TODO: Fabian fragen, ob diese Impl so passt.

  container_type Gen;

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    const auto PostProcessFacts = [AS = AS, Call](container_type &Facts) {
      populateWithMayAliases(AS, Facts, Call);
    };

    if (const auto *Return = llvm::dyn_cast<llvm::ReturnInst>(Call)) {
      container_type Gen;

      auto AliasSet =
          AS.getReachableAllocationSites(Return->getReturnValue(), Return);
      Gen.insert(AliasSet->begin(), AliasSet->end());

      return FFTemplates::lambdaFlow(
          [Return, Gen{std::move(Gen)}, AS = AS](d_t Source) -> container_type {
            if (Return->getReturnValue() == Source ||
                AS.isInReachableAllocationSites(Return->getReturnValue(),
                                                Source, true, Return)) {
              return Gen;
            }

            return {Source};
          });
    }

    return mapFactsToCaller(
        Call, ExitInst,
        [AS = AS](d_t Param, d_t Source) {
          return (Param == Source && Param->getType()->isPointerTy()) ||
                 AS.isInReachableAllocationSites(Param, Source);
        },
        {}, {}, true, true, PostProcessFacts);
  }

  return FFTemplates::killAllFlows();
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getCallToRetFlowFunctionImpl(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees)
        -> FlowFunctionPtrType {
  return this->IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl(
      CallSite, RetSite, Callees);
}
