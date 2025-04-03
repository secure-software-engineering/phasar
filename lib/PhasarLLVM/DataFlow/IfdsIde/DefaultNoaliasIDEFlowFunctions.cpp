#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

using namespace psr;

using FFTemplates =
    FlowFunctionTemplates<detail::IDENoAliasDefaultFlowFunctionsImpl::d_t,
                          detail::IDENoAliasDefaultFlowFunctionsImpl::
                              FlowFunctionType::container_type>;

bool detail::IDENoAliasDefaultFlowFunctionsImpl::isFunctionModeled(
    f_t Fun) const {
  return !Fun->isDeclaration();
}

auto detail::IDENoAliasDefaultFlowFunctionsImpl::getNormalFlowFunctionImpl(
    n_t Curr, n_t /*Succ*/) -> FlowFunctionPtrType {

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return strongUpdateStore(Store);
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return FFTemplates::generateFlowIf(Load, [Load](d_t Source) {
      return Source == Load->getPointerOperand();
    });
  }
  if (const auto *UnaryOp = llvm::dyn_cast<llvm::UnaryOperator>(Curr)) {
    return FFTemplates::generateFlow(UnaryOp, UnaryOp->getOperand(0));
  }
  if (const auto *BinaryOp = llvm::dyn_cast<llvm::BinaryOperator>(Curr)) {
    return FFTemplates::generateFlowIf(BinaryOp, [BinaryOp](d_t Source) {
      return Source == BinaryOp->getOperand(0) ||
             Source == BinaryOp->getOperand(1);
    });
  }
  if (const auto *GetElementPtr = llvm::dyn_cast<llvm::GEPOperator>(Curr)) {
    return FFTemplates::generateFlow(GetElementPtr,
                                     GetElementPtr->getPointerOperand());
  }

  return FFTemplates::identityFlow();
}

auto detail::IDENoAliasDefaultFlowFunctionsImpl::getCallFlowFunctionImpl(
    n_t CallInst, f_t CalleeFun) -> FlowFunctionPtrType {
  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(CallInst)) {
    return mapFactsToCallee(CallSite, CalleeFun);
  }

  return FFTemplates::killAllFlows();
}

auto detail::IDENoAliasDefaultFlowFunctionsImpl::getRetFlowFunctionImpl(
    n_t CallSite, f_t /*CalleeFun*/, n_t ExitInst, n_t /*RetSite*/)
    -> FlowFunctionPtrType {

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return mapFactsToCaller(Call, ExitInst, [](d_t Param, d_t Source) {
      return Param == Source && Param->getType()->isPointerTy();
    });
  }

  return FFTemplates::killAllFlows();
}

auto detail::IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> Callees)
    -> FlowFunctionPtrType {

  if (llvm::any_of(Callees,
                   [this](f_t Callee) { return !isFunctionModeled(Callee); })) {
    // Cannot strongly update, if we don't know the callee
    return FFTemplates::identityFlow();
  }

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return mapFactsAlongsideCallSite(
        Call, [](d_t Arg) { return !Arg->getType()->isPointerTy(); }, false);
  }

  return FFTemplates::identityFlow();
}
