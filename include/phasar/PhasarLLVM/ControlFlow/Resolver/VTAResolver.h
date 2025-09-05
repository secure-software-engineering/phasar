/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_VTARESOLVER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_VTARESOLVER_H

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypePropagator.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace psr {
class VTAResolver : public Resolver {
public:
  struct DefaultReachableFunctions {
    void operator()(const LLVMProjectIRDB &IRDB,
                    llvm::function_ref<void(const llvm::Function *)> WithFun);
  };

  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP, vta::AliasInfoTy AS,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG);
  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP, LLVMAliasInfoRef AS,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG)
      : VTAResolver(
            IRDB, VTP,
            [AS](const llvm::Value *Ptr, const llvm::Instruction *At,
                 vta::AliasHandlerTy WithAlias) {
              auto ASet = AS.getAliasSet(Ptr, At);
              llvm::for_each(*ASet, WithAlias);
            },
            std::move(BaseCG)) {}

  explicit VTAResolver(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      vta::AliasInfoTy AS, MaybeUniquePtr<Resolver> BaseRes,
      llvm::function_ref<void(const LLVMProjectIRDB &,
                              llvm::function_ref<void(const llvm::Function *)>)>
          ReachableFunctions = DefaultReachableFunctions{});
  explicit VTAResolver(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      LLVMAliasInfoRef AS, MaybeUniquePtr<Resolver> BaseRes,
      llvm::function_ref<void(const LLVMProjectIRDB &,
                              llvm::function_ref<void(const llvm::Function *)>)>
          ReachableFunctions = DefaultReachableFunctions{})
      : VTAResolver(
            IRDB, VTP,
            [AS](const llvm::Value *Ptr, const llvm::Instruction *At,
                 vta::AliasHandlerTy WithAlias) {
              auto ASet = AS.getAliasSet(Ptr, At);
              llvm::for_each(*ASet, WithAlias);
            },
            std::move(BaseRes), ReachableFunctions) {}

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return false;
  }

private:
  void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                          const llvm::CallBase *CallSite) override;

  void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                              const llvm::CallBase *CallSite) override;

  MaybeUniquePtr<Resolver> BaseCG{};
  vta::TypeAssignment TA{};
  SCCHolder<vta::TAGNodeId> SCCs{};
  Compressor<vta::TAGNode, vta::TAGNodeId> Nodes;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_VTARESOLVER_H
