/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_PRECOMPUTEDRESOLVER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_PRECOMPUTEDRESOLVER_H

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/Utils/MaybeUniquePtr.h"

namespace psr {
/// \brief A Resolver that uses a pre-computed call-graph to resolve indirect
/// calls.
///
/// \note We eventually may want the LLVMBasedCallGraph to *be* a Resolver. This
/// requires the concept of resolvers to generalize beyond LLVM. See
/// <https://github.com/fabianbs96/phasar/tree/f-ResolverCombinators> for
/// reference
class PrecomputedResolver : public Resolver {
public:
  PrecomputedResolver(const LLVMProjectIRDB *IRDB,
                      const LLVMVFTableProvider *VTP,
                      MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG);

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return false;
  }

  void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                          const llvm::CallBase *CallSite) override {
    resolveFunctionPointer(PossibleTargets, CallSite);
  }

  void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                              const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

private:
  MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG;
};
} // namespace psr

#endif
