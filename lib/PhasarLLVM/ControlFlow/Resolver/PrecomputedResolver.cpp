#include "phasar/PhasarLLVM/ControlFlow/Resolver/PrecomputedResolver.h"

#include "llvm/IR/InstrTypes.h"

using namespace psr;

PrecomputedResolver::PrecomputedResolver(
    const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
    MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG)
    : Resolver(IRDB, VTP), BaseCG(std::move(BaseCG)) {
  assert(this->BaseCG != nullptr);
}

void PrecomputedResolver::resolveFunctionPointer(
    FunctionSetTy &PossibleTargets, const llvm::CallBase *CallSite) {
  auto Callees = BaseCG->getCalleesOfCallAt(CallSite);
  PossibleTargets.insert(Callees.begin(), Callees.end());
}

std::string PrecomputedResolver::str() const { return "Precomputed"; }
