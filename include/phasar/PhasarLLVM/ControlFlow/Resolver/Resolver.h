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

[[nodiscard]] std::optional<unsigned>
getVFTIndex(const llvm::CallBase *CallSite);

[[nodiscard]] const llvm::DIType *
getReceiverType(const llvm::CallBase *CallSite);

[[nodiscard]] std::string getReceiverTypeName(const llvm::CallBase *CallSite);

[[nodiscard]] bool isConsistentCall(const llvm::CallBase *CallSite,
                                    const llvm::Function *DestFun);

[[nodiscard]] bool isVirtualCall(const llvm::Instruction *Inst,
                                 const LLVMVFTableProvider &VTP);

class Resolver {
protected:
  const LLVMProjectIRDB *IRDB;
  const LLVMVFTableProvider *VTP;

  const llvm::Function *
  getNonPureVirtualVFTEntry(const llvm::DIType *T, unsigned Idx,
                            const llvm::CallBase *CallSite);

public:
  using FunctionSetTy = llvm::SmallDenseSet<const llvm::Function *, 4>;

  Resolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP);

  virtual ~Resolver() = default;

  virtual void preCall(const llvm::Instruction *Inst);

  virtual void handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets);

  virtual void postCall(const llvm::Instruction *Inst);

  [[nodiscard]] FunctionSetTy
  resolveIndirectCall(const llvm::CallBase *CallSite);

  [[nodiscard]] virtual FunctionSetTy
  resolveVirtualCall(const llvm::CallBase *CallSite) = 0;

  [[nodiscard]] virtual FunctionSetTy
  resolveFunctionPointer(const llvm::CallBase *CallSite);

  virtual void otherInst(const llvm::Instruction *Inst);

  [[nodiscard]] virtual std::string str() const = 0;

  [[nodiscard]] virtual bool mutatesHelperAnalysisInformation() const noexcept {
    // Conservatively returns true. Override if possible
    return true;
  }
  static std::unique_ptr<Resolver> create(CallGraphAnalysisType Ty,
                                          const LLVMProjectIRDB *IRDB,
                                          const LLVMVFTableProvider *VTP,
                                          const DIBasedTypeHierarchy *TH,
                                          LLVMAliasInfoRef PT = nullptr);
};
} // namespace psr

#endif
