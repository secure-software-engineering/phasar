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

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypePropagator.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/ADT/STLExtras.h"

namespace psr {
class VTAResolver : public Resolver {
public:
  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG,
                       vta::AliasInfoTy AS);
  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG,
                       LLVMAliasInfoRef AS)
      : VTAResolver(IRDB, VTP, std::move(BaseCG),
                    [AS](const llvm::Value *Ptr, const llvm::Instruction *At,
                         vta::AliasHandlerTy WithAlias) {
                      auto ASet = AS.getAliasSet(Ptr, At);
                      llvm::for_each(*ASet, WithAlias);
                    }) {}

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

  MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG{};
  vta::TypeAssignment TA{};
  SCCHolder<vta::TAGNodeId> SCCs{};
  Compressor<vta::TAGNode, vta::TAGNodeId> Nodes;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_VTARESOLVER_H
