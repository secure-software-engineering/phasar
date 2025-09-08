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

  [[deprecated("With the removal of DTAResolver, this is not used "
               "anymore")]] virtual void
  preCall(const llvm::Instruction *Inst);

  virtual void handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets);

  [[deprecated("With the removal of DTAResolver, this is not used "
               "anymore")]] virtual void
  postCall(const llvm::Instruction *Inst);

  [[nodiscard]] FunctionSetTy
  resolveIndirectCall(const llvm::CallBase *CallSite);

  [[deprecated("With the removal of DTAResolver, this is not used "
               "anymore")]] virtual void
  otherInst(const llvm::Instruction *Inst);

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

  [[nodiscard]] static std::unique_ptr<Resolver>
  create(CallGraphAnalysisType Ty, const LLVMProjectIRDB *IRDB,
         const LLVMVFTableProvider *VTP, const DIBasedTypeHierarchy *TH,
         LLVMAliasInfoRef PT = nullptr);

protected:
  const llvm::Function *
  getNonPureVirtualVFTEntry(const llvm::DIType *T, unsigned Idx,
                            const llvm::CallBase *CallSite,
                            const llvm::DIType *ReceiverType) {
    if (!VTP) {
      return nullptr;
    }
    return psr::getNonPureVirtualVFTEntry(T, Idx, CallSite, *VTP, ReceiverType);
  }

  const LLVMProjectIRDB *IRDB{};
  const LLVMVFTableProvider *VTP{};
  std::optional<llvm::SmallVector<const llvm::Function *, 0>>
      AddressTakenFunctions{};

protected:
  virtual void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                  const llvm::CallBase *CallSite) = 0;

  virtual void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                                      const llvm::CallBase *CallSite);
};
} // namespace psr

#endif
