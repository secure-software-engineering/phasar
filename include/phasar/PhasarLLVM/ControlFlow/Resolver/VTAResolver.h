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

#include "llvm/ADT/STLFunctionalExtras.h"

namespace psr {
///\brief A Resolver that uses a variant of the Variable Type Analysis to
/// resolver indirect calls.
///
/// Uses debug-information to achieve better results with C++ virtual calls.
/// Uses alias-information as fallback mechanism for when types don't help or
/// are not found, e.g., to resolve function-pointer calls.
///
/// Requires a base-call-graph or at least a base-resolver to resolve indirect
/// calls while constructing the type-assignment graph.
class VTAResolver : public Resolver {
public:
  struct DefaultReachableFunctions {
    void operator()(const LLVMProjectIRDB &IRDB,
                    llvm::function_ref<void(const llvm::Function *)> WithFun);
  };

  /// Constructs a VTAResolver with a given pre-computed call-graph and
  /// call-back based alias-information (to-be-replaced by AliasIterator once
  /// available #783)
  ///
  /// Builds the type-assignment graph and propagates allocated types though
  /// it's SCCs.
  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP, vta::AliasInfoTy AS,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG);

  /// Constructs a VTAResolver with a given pre-computed call-graph and
  /// LLVMAliasInfoRef alias-information.
  ///
  /// Builds the type-assignment graph and propagates allocated types though
  /// it's SCCs.
  explicit VTAResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP, LLVMAliasInfoRef AS,
                       MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG);

  /// Constructs a VTAResolver with a given base-resolver (no base-call-graph)
  /// and call-back based alias-information (to-be-replaced by AliasIterator
  /// once available #783).
  /// Uses the optional parameter ReachableFunctions to consider only a subset
  /// of all functions for building the type-assignment graph
  ///
  /// Builds the type-assignment graph and propagates allocated types though
  /// it's SCCs.
  explicit VTAResolver(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      vta::AliasInfoTy AS, MaybeUniquePtr<Resolver> BaseRes,
      llvm::function_ref<void(const LLVMProjectIRDB &,
                              llvm::function_ref<void(const llvm::Function *)>)>
          ReachableFunctions = DefaultReachableFunctions{});

  /// Constructs a VTAResolver with a given base-resolver (no base-call-graph)
  /// and LLVMAliasInfoRef alias-information.
  /// Uses the optional parameter ReachableFunctions to consider only a subset
  /// of all functions for building the type-assignment graph
  ///
  /// Builds the type-assignment graph and propagates allocated types though
  /// it's SCCs.
  explicit VTAResolver(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      LLVMAliasInfoRef AS, MaybeUniquePtr<Resolver> BaseRes,
      llvm::function_ref<void(const LLVMProjectIRDB &,
                              llvm::function_ref<void(const llvm::Function *)>)>
          ReachableFunctions = DefaultReachableFunctions{});

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

  MaybeUniquePtr<Resolver> BaseResolver{};
  vta::TypeAssignment TA{};
  SCCHolder<vta::TAGNodeId> SCCs{};
  Compressor<vta::TAGNode, vta::TAGNodeId> Nodes;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_VTARESOLVER_H
