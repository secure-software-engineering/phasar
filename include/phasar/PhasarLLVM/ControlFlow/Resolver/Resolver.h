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

#pragma once

#include <string>
#include <set>

namespace llvm {
  class Instruction;
  class ImmutableCallSite;
  class Function;
}

namespace psr {
  class ProjectIRDB;
  class LLVMTypeHierarchy;

  struct Resolver {
  protected:
    ProjectIRDB &IRDB;
    LLVMTypeHierarchy &CH;

  protected:
    unsigned int getVtableIndex(const llvm::ImmutableCallSite &CS) const;
    std::string getReceiverTypeName(const llvm::ImmutableCallSite &CS) const;
    void insertVtableIntoResult(std::set<std::string> &results, std::string &struct_name, unsigned vtable_index);

  public:
    Resolver(ProjectIRDB &irdb, const LLVMTypeHierarchy &ch);

    virtual void firstFunction(const llvm::Function* F);
    virtual void preCall(const llvm::Instruction* inst);
    virtual void postCall(const llvm::Instruction* inst);
    virtual void OtherInst(const llvm::Instruction* inst);
    virtual std::set<std::string> resolveVirtualCall(const llvm::ImmutableCallSite &CS) = 0;
    virtual std::set<std::string> resolveFunctionPointer(const llvm::ImmutableCallSite &CS);
  };
} // namespace psr
