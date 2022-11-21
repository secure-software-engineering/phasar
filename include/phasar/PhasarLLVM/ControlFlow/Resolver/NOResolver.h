/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_NORESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_NORESOLVER_H_

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"

namespace llvm {
class Instruction;
class CallBase;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class NOResolver final : public Resolver {
protected:
  const llvm::Function *
  getNonPureVirtualVFTEntry(const llvm::StructType *T, unsigned Idx,
                            const llvm::CallBase *CallSite);

public:
  NOResolver(ProjectIRDB &IRDB);

  ~NOResolver() override = default;

  void preCall(const llvm::Instruction *Inst) override;

  void handlePossibleTargets(const llvm::CallBase *CallSite,
                             FunctionSetTy &PossibleTargets) override;

  void postCall(const llvm::Instruction *Inst) override;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  FunctionSetTy resolveFunctionPointer(const llvm::CallBase *CallSite) override;

  void otherInst(const llvm::Instruction *Inst) override;

  [[nodiscard]] std::string str() const override;
};
} // namespace psr

#endif
