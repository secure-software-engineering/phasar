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
class CallBase;
class Function;
class StructType;
} // namespace llvm

namespace psr {
class ProjectIRDB;
class LLVMTypeHierarchy;

int getVFTIndex(const llvm::CallBase *CB);

const llvm::StructType *getReceiverType(const llvm::CallBase *CB);

std::string getReceiverTypeName(const llvm::CallBase &CB);

class Resolver {
protected:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy *TH;

  Resolver(ProjectIRDB &IRDB);

  const llvm::Function *getNonPureVirtualVFTEntry(const llvm::StructType *T,
                                                  unsigned Idx,
                                                  const llvm::CallBase *CB);

public:
  Resolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  virtual ~Resolver() = default;

  virtual void preCall(const llvm::Instruction *Inst);

  virtual void
  handlePossibleTargets(const llvm::CallBase *CB,
                        std::set<const llvm::Function *> &PossibleTargets);

  virtual void postCall(const llvm::Instruction *Inst);

  virtual std::set<const llvm::Function *>
  resolveVirtualCall(const llvm::CallBase *CB) = 0;

  virtual std::set<const llvm::Function *>
  resolveFunctionPointer(const llvm::CallBase *CB);

  virtual void otherInst(const llvm::Instruction *Inst);
};
} // namespace psr

#endif
