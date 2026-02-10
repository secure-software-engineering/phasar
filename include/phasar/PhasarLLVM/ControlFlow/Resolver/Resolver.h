/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Resolver.h
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_RESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_RESOLVER_H_

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DerivedTypes.h"

#include <memory>
#include <optional>
#include <string>

namespace llvm {
class Instruction;
class CallBase;
class Function;
class DIType;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;
class LLVMVFTableProvider;
class DIBasedTypeHierarchy;
enum class CallGraphAnalysisType;

/// Assuming that `CallSite` is a virtual call through a vtable, retrieves the
/// index in the vtable of the virtual function called.
[[nodiscard]] std::optional<unsigned>
getVFTIndex(const llvm::CallBase *CallSite);

/// Similar to getVFTIndex(), but also returns a pointer to the vtable
[[nodiscard]] std::optional<std::pair<const llvm::Value *, uint64_t>>
getVFTIndexAndVT(const llvm::CallBase *CallSite);

/// Assuming that `CallSite` is a call to a non-static member function,
/// retrieves the type of the receiver. Returns nullptr, if the receiver-type
/// could not be extracted
[[nodiscard]] const llvm::DIType *
getReceiverType(const llvm::CallBase *CallSite);

/// Assuming that `CallSite` is a virtual call, where `Idx` is retrieved through
/// `getVFTIndex()` and `T` through `getReceiverType()`
[[nodiscard]] const llvm::Function *getNonPureVirtualVFTEntry(
    const llvm::DIType *T, unsigned Idx, const llvm::CallBase *CallSite,
    const psr::LLVMVFTableProvider &VTP, const llvm::DIType *ReceiverType);

[[nodiscard]] std::string getReceiverTypeName(const llvm::CallBase *CallSite);

/// Checks whether the signature of `DestFun` matches the required withature of
/// `CallSite`, such that `DestFun` qualifies as callee-candidate, if `CallSite`
/// is an indirect/virtual call.
[[nodiscard]] bool isConsistentCall(const llvm::CallBase *CallSite,
                                    const llvm::Function *DestFun);

[[nodiscard]] bool isVirtualCall(const llvm::Instruction *Inst,
                                 const LLVMVFTableProvider &VTP);

/// A variant of F->hasAddressTaken() that is better suited for our use cases.
///
/// Especially, it filteres out global aliases.
[[nodiscard]] bool isAddressTakenFunction(const llvm::Function *F);

/// \brief A base class for call-target resolvers. Used to build call graphs.
///
/// Create a specific resolver by making a new class, inheriting this resolver
/// class and implementing the virtual functions as needed.
class Resolver {
public:
  using FunctionSetTy = llvm::SmallDenseSet<const llvm::Function *, 4>;

  Resolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP);

  virtual ~Resolver() = default;

  virtual void handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets);

  [[nodiscard]] FunctionSetTy
  resolveIndirectCall(const llvm::CallBase *CallSite);

  [[nodiscard]] virtual std::string str() const = 0;

  /// Whether the ICFG needs to reconsider all dynamic call-sites once there
  /// have been changes through handlePossibleTargets().
  ///
  /// Make false for performance (may be less sound then)
  [[nodiscard]] virtual bool mutatesHelperAnalysisInformation() const noexcept {
    // Conservatively returns true. Override if possible
    return true;
  }

  [[nodiscard]] llvm::ArrayRef<const llvm::Function *>
  getAddressTakenFunctions();

  using BaseResolverProvider = llvm::function_ref<MaybeUniquePtr<Resolver>(
      const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
      const DIBasedTypeHierarchy *TH, LLVMAliasInfoRef PT)>;

  /// Factory function to create a Resolver that can be used to implement the
  /// given call-graph analysis type.
  ///
  /// \param Ty Determines the Resolver subclass to instantiate
  /// \param IRDB The IR code where the Resolver should be based on. Must not be
  /// nullptr.
  /// \param VTP A virtual-table-provider that is used to extract C++-VTables
  /// from the IR. Must not be nullptr.
  /// \param TH The type-hierarchy implementation to use. Must be non-null, if
  /// the selected call-graph analysis requires type-hierarchy information;
  /// currently, this holds for the CHA and RTA algorithms.
  /// \param PT The points-to implementation to use. Will be constructed
  /// on-the-fly if nullptr, but required; currently, this holds for the OTF and
  /// VTA algorithms.
  static std::unique_ptr<Resolver>
  create(CallGraphAnalysisType Ty, const LLVMProjectIRDB *IRDB,
         const LLVMVFTableProvider *VTP, const DIBasedTypeHierarchy *TH,
         LLVMAliasInfoRef PT = nullptr,
         BaseResolverProvider GetBaseRes = nullptr);

protected:
  virtual void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                  const llvm::CallBase *CallSite) = 0;

  virtual void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                                      const llvm::CallBase *CallSite);

  const llvm::Function *
  getNonPureVirtualVFTEntry(const llvm::DIType *T, unsigned Idx,
                            const llvm::CallBase *CallSite,
                            const llvm::DIType *ReceiverType) {
    if (!VTP) {
      return nullptr;
    }
    return psr::getNonPureVirtualVFTEntry(T, Idx, CallSite, *VTP, ReceiverType);
  }

  // ---

  const LLVMProjectIRDB *IRDB{};
  const LLVMVFTableProvider *VTP{};
  std::optional<llvm::SmallVector<const llvm::Function *, 0>>
      AddressTakenFunctions{};
};
} // namespace psr

#endif
