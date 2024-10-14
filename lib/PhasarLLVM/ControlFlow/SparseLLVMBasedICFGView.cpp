#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFGView.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "SVFGCache.h"

using namespace psr;

SparseLLVMBasedICFGView::SparseLLVMBasedICFGView(const LLVMBasedICFG *ICF)
    : IRDB(ICF->getIRDB()), ICF(ICF), SparseCFGCache(new SVFGCache{}) {
  //
}

SparseLLVMBasedICFGView::~SparseLLVMBasedICFGView() = default;

FunctionRange SparseLLVMBasedICFGView::getAllFunctionsImpl() const {
  return IRDB->getAllFunctions();
}

auto SparseLLVMBasedICFGView::getFunctionImpl(llvm::StringRef Fun) const
    -> f_t {
  return IRDB->getFunction(Fun);
};

bool SparseLLVMBasedICFGView::isIndirectFunctionCallImpl(n_t Inst) const {
  return ICF->isIndirectFunctionCall(Inst);
}

bool SparseLLVMBasedICFGView::isVirtualFunctionCallImpl(n_t Inst) const {
  return ICF->isVirtualFunctionCall(Inst);
}

auto SparseLLVMBasedICFGView::allNonCallStartNodesImpl() const
    -> std::vector<n_t> {
  return ICF->allNonCallStartNodes();
}

auto SparseLLVMBasedICFGView::getCallsFromWithinImpl(f_t Fun) const
    -> llvm::SmallVector<n_t> {
  return ICF->getCallsFromWithin(Fun);
}

auto SparseLLVMBasedICFGView::getReturnSitesOfCallAtImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  return ICF->getReturnSitesOfCallAt(Inst);
}

void SparseLLVMBasedICFGView::printImpl(llvm::raw_ostream &OS) const {
  ICF->print(OS);
}

nlohmann::json SparseLLVMBasedICFGView::getAsJsonImpl() const {
  return ICF->getAsJson();
}

auto SparseLLVMBasedICFGView::getCallGraphImpl() const noexcept
    -> const CallGraph<n_t, f_t> & {
  return ICF->getCallGraph();
}

const SparseLLVMBasedCFG &
SparseLLVMBasedICFGView::getSparseCFGImpl(const llvm::Function *Fun,
                                          const llvm::Value *Val) const {
  assert(SparseCFGCache != nullptr);
  return SparseCFGCache->getOrCreate(*this, Fun, Val);
}
