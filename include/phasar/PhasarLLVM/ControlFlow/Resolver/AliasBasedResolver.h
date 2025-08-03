/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_ALIASBASEDRESOLVER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_ALIASBASEDRESOLVER_H

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace llvm {
class CallBase;
class Function;
class Value;
} // namespace llvm

namespace psr {
/// \brief A resolver that uses alias-information to resolve indirect and
/// virtual calls; in contrast to the OTFResolver, the alias-information is
/// fixed and this resolver does *not* attempt to modify the alias info during
/// call-graph construction.
class AliasBasedResolver : public Resolver {
public:
  explicit AliasBasedResolver(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      llvm::unique_function<void(const llvm::Value *, const llvm::Instruction *,
                                 llvm::function_ref<void(const llvm::Value *)>)>
          &&ForAllAliasesOf,
      Resolver *FallbackResolver = nullptr);

  explicit AliasBasedResolver(const LLVMProjectIRDB *IRDB,
                              const LLVMVFTableProvider *VTP,
                              LLVMAliasInfoRef AS,
                              Resolver *FallbackResolver = nullptr);

protected:
  void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                          const llvm::CallBase *CallSite) override;

  void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                              const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return false;
  }

private:
  // Should be replaced by LLVMAliasIteratorRef once #783 is merged!
  llvm::unique_function<void(const llvm::Value *, const llvm::Instruction *,
                             llvm::function_ref<void(const llvm::Value *)>)>
      ForAllAliasesOf;
  Resolver *FallbackResolver = nullptr;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_ALIASBASEDRESOLVER_H
