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

#include <set>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

namespace llvm {
class Instruction;
class ImmutableCallSite;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class NOResolver final : public Resolver {
protected:
  const llvm::Function *getNonPureVirtualVFTEntry(const llvm::StructType *T,
                                                  unsigned Idx,
                                                  llvm::ImmutableCallSite CS);

public:
  NOResolver(ProjectIRDB &IRDB);

  ~NOResolver() override = default;

  void preCall(const llvm::Instruction *Inst) override;

  void handlePossibleTargets(
      llvm::ImmutableCallSite CS,
      std::set<const llvm::Function *> &PossibleTargets) override;

  void postCall(const llvm::Instruction *Inst) override;

  std::set<const llvm::Function *>
  resolveVirtualCall(llvm::ImmutableCallSite CS) override;

  std::set<const llvm::Function *>
  resolveFunctionPointer(llvm::ImmutableCallSite CS) override;

  void otherInst(const llvm::Instruction *Inst) override;
};
} // namespace psr

#endif
