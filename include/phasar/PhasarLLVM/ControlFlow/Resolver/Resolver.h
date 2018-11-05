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

class Resolver {
protected:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy &CH;

protected:
  int getVtableIndex(const llvm::ImmutableCallSite &CS) const;
  const llvm::StructType *
  getReceiverType(const llvm::ImmutableCallSite &CS) const;
  std::string getReceiverTypeName(const llvm::ImmutableCallSite &CS) const;
  void insertVtableIntoResult(std::set<std::string> &results,
                              const std::string &struct_name,
                              const unsigned vtable_index,
                              const llvm::ImmutableCallSite &CS);
  bool matchVirtualSignature(const llvm::FunctionType *type_call,
                             const llvm::FunctionType *type_candidate);

public:
  Resolver(ProjectIRDB &DB, LLVMTypeHierarchy &H);

  virtual ~Resolver() = default;

  virtual void firstFunction(const llvm::Function *F);
  virtual void preCall(const llvm::Instruction *Inst);
  virtual void
  TreatPossibleTarget(const llvm::ImmutableCallSite &CS,
                      std::set<const llvm::Function *> &PossibleTargets);
  virtual void postCall(const llvm::Instruction *Inst);
  virtual void OtherInst(const llvm::Instruction *Inst);
  virtual std::set<std::string>
  resolveVirtualCall(const llvm::ImmutableCallSite &CS) = 0;
  virtual std::set<std::string>
  resolveFunctionPointer(const llvm::ImmutableCallSite &CS);
};
} // namespace psr

#endif
