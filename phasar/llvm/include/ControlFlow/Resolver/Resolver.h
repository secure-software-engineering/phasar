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

#include <optional>
#include <set>
#include <string>

#include "llvm/ADT/DenseSet.h"

namespace llvm {
class Instruction;
class CallBase;
class Function;
class StructType;
} // namespace llvm

namespace psr {
class ProjectIRDB;
class LLVMTypeHierarchy;

std::optional<unsigned> getVFTIndex(const llvm::CallBase *CallSite);

const llvm::StructType *getReceiverType(const llvm::CallBase *CallSite);

std::string getReceiverTypeName(const llvm::CallBase &CallSite);

class Resolver {
protected:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy *TH;

  Resolver(ProjectIRDB &IRDB);

  const llvm::Function *
  getNonPureVirtualVFTEntry(const llvm::StructType *T, unsigned Idx,
                            const llvm::CallBase *CallSite);

public:
  using FunctionSetTy = llvm::SmallDenseSet<const llvm::Function *, 4>;

  Resolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  virtual ~Resolver() = default;

  virtual void preCall(const llvm::Instruction *Inst);

  virtual void handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets);

  virtual void postCall(const llvm::Instruction *Inst);

  virtual FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) = 0;

  virtual FunctionSetTy resolveFunctionPointer(const llvm::CallBase *CallSite);

  virtual void otherInst(const llvm::Instruction *Inst);
};
} // namespace psr

#endif
