#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultReachableAllocationSitesIDEProblem.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"

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

    auto Gen = AS.asSet<container_type>(Store->getPointerOperand(), Store);

    return FFTemplates::lambdaFlow(
        [Store, Gen{std::move(Gen)}, AS = AS](d_t Source) -> container_type {
          if (Store->getPointerOperand() == Source) {
            return {};
          }
          if (Store->getValueOperand() == Source ||
              AS.mayPointsTo(Store->getValueOperand(), Source, Store)) {
            auto Ret = Gen;
            Ret.insert(Source);
            return Ret;
          }

          return {Source};
        });
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return FFTemplates::lambdaFlow(
        [Load, AS = AS](d_t Source) -> container_type {
          if (Load->getPointerOperand() == Source ||
              AS.mayPointsTo(Load->getPointerOperand(), Source, Load)) {
            return {Source, Load};
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

  return mapFactsToCallee(Call, CalleeFun,
                          [Call, AS = AS](d_t Arg, d_t Source) -> bool {
                            if (Arg == Source) {
                              return true;
                            }

                            return Arg->getType()->isPointerTy() &&
                                   Source->getType()->isPointerTy() &&
                                   AS.mayPointsTo(Arg, Source, Call);
                          });
}

static void populateWithMayPointees(LLVMPointsToIteratorRef AS,
                                    container_type &Facts,
                                    const llvm::Instruction *Context) {
  container_type Tmp = Facts;
  for (const auto *Fact : Tmp) {
    AS.forallPointeesOf(Fact, Context, [&Facts](const auto *Pointee) {
      Facts.insert(Pointee);
    });
  }
}

auto detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl::
    getRetFlowFunctionImpl(n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst,
                           n_t /*RetSite*/) -> FlowFunctionPtrType {
  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  const auto PostProcessFacts = [AS = AS, Call](container_type &Facts) {
    populateWithMayPointees(AS, Facts, Call);
  };

  return mapFactsToCaller(
      Call, ExitInst,
      [AS = AS, ExitInst](d_t Param, d_t Source) {
        if (!Param->getType()->isPointerTy()) {
          return false;
        }

        if (Param == Source) {
          return true;
        }

        // Arguments are counted as allocation-sites, so we have generated them
        // as aliases
        return !llvm::isa<llvm::Argument>(Source) &&
               AS.mayPointsTo(Param, Source, ExitInst);
      },
      [AS = AS, ExitInst](d_t Ret, d_t Source) {
        if (Ret == Source) {
          return true;
        }

        return Ret->getType()->isPointerTy() &&
               Source->getType()->isPointerTy() &&
               AS.mayPointsTo(Ret, Source, ExitInst);
      },
      {}, true, true, PostProcessFacts);
}
