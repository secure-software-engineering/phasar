#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAliasAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/Instructions.h"

using namespace psr;

using FFTemplates =
    FlowFunctionTemplates<detail::IDEAliasAwareDefaultFlowFunctionsImpl::d_t,
                          detail::IDEAliasAwareDefaultFlowFunctionsImpl::
                              FlowFunctionType::container_type>;
using container_type = FFTemplates::container_type;

auto detail::IDEAliasAwareDefaultFlowFunctionsImpl::getNormalFlowFunctionImpl(
    n_t Curr, n_t Succ) -> FlowFunctionPtrType {

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    container_type Gen;

    auto AliasSet = AS.getAliasSet(Store->getPointerOperand(), Store);
    Gen.insert(AliasSet->begin(), AliasSet->end());
    Gen.insert(Store->getValueOperand());

    return FFTemplates::lambdaFlow(
        [Store, Gen{std::move(Gen)}](d_t Source) -> container_type {
          if (Store->getPointerOperand() == Source) {
            return {};
          }
          if (Store->getValueOperand() == Source) {
            return Gen;
          }

          return {Source};
        });
  }

  return this->IDENoAliasDefaultFlowFunctionsImpl::getNormalFlowFunctionImpl(
      Curr, Succ);
}

auto detail::IDEAliasAwareDefaultFlowFunctionsImpl::getCallFlowFunctionImpl(
    n_t CallInst, f_t CalleeFun) -> FlowFunctionPtrType {
  return this->IDENoAliasDefaultFlowFunctionsImpl::getCallFlowFunctionImpl(
      CallInst, CalleeFun);
}

static void populateWithMayAliases(LLVMAliasInfoRef AS, container_type &Facts,
                                   const llvm::Instruction *Context) {
  container_type Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = AS.getAliasSet(Fact, Context);
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

auto detail::IDEAliasAwareDefaultFlowFunctionsImpl::getRetFlowFunctionImpl(
    n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst, n_t /*RetSite*/)
    -> FlowFunctionPtrType {
  container_type Gen;

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    const auto PostProcessFacts = [AS = AS, Call](container_type &Facts) {
      populateWithMayAliases(AS, Facts, Call);
    };

    return mapFactsToCaller(
        Call, ExitInst,
        [](d_t Param, d_t Source) {
          return Param == Source && Param->getType()->isPointerTy();
        },
        {}, {}, true, true, PostProcessFacts);
  }

  return FFTemplates::killAllFlows();
}

auto detail::IDEAliasAwareDefaultFlowFunctionsImpl::
    getCallToRetFlowFunctionImpl(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees)
        -> FlowFunctionPtrType {
  return this->IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl(
      CallSite, RetSite, Callees);
}
