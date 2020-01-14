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

#include <set>
#include <string>

namespace llvm {
class Instruction;
class ImmutableCallSite;
class Function;
class StructType;
} // namespace llvm

namespace psr {
class ProjectIRDB;
class LLVMTypeHierarchy;

int getVFTIndex(const llvm::ImmutableCallSite CS);

const llvm::StructType *getReceiverType(llvm::ImmutableCallSite CS);

std::string getReceiverTypeName(llvm::ImmutableCallSite CS);

class Resolver {
protected:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy &TH;

protected:
  const llvm::Function *getNonPureVirtualVFTEntry(const llvm::StructType *T,
                                                  unsigned Idx,
                                                  llvm::ImmutableCallSite CS);

public:
  Resolver(ProjectIRDB &DB, LLVMTypeHierarchy &H);

  virtual ~Resolver() = default;

  virtual void firstFunction(const llvm::Function *F);
  virtual void preCall(const llvm::Instruction *Inst);
  virtual void
  TreatPossibleTarget(llvm::ImmutableCallSite CS,
                      std::set<const llvm::Function *> &PossibleTargets);
  virtual void postCall(const llvm::Instruction *Inst);
  virtual void OtherInst(const llvm::Instruction *Inst);
  virtual std::set<const llvm::Function *>
  resolveVirtualCall(llvm::ImmutableCallSite CS) = 0;
  virtual std::set<const llvm::Function *>
  resolveFunctionPointer(llvm::ImmutableCallSite CS);
};
} // namespace psr

#endif
