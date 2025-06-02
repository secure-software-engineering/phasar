#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultReachableAllocationSitesIDEProblem.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"

#include "llvm/IR/Instructions.h"

#include <cstdlib>

using namespace psr;

using FFTemplates = FlowFunctionTemplates<
    detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::d_t,
    detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
        FlowFunctionType::container_type>;
using container_type = FFTemplates::container_type;

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getNormalFlowFunctionImpl(n_t Curr, n_t Succ) -> FlowFunctionPtrType {
  abort();
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getCallFlowFunctionImpl(n_t CallInst, f_t CalleeFun)
        -> FlowFunctionPtrType {
  abort();
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
  abort();
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getCallToRetFlowFunctionImpl(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees)
        -> FlowFunctionPtrType {
  abort();
}
