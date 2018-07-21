/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * OTFResolver.h
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#pragma once

#include <string>
#include <set>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

namespace llvm {
  class Instruction;
  class ImmutableCallSite;
}

namespace psr {
  class ProjectIRDB;

  struct OTFResolver {
  protected:

  public:
    OTFResolver(ProjectIRDB &irdb);

    virtual void preCall(const llvm::Instruction* inst) override;
    virtual void postCall(const llvm::Instruction* inst) override;
    virtual void OtherInst(const llvm::Instruction* inst) override;
    virtual std::set<std::string> resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
  };
} // namespace psr
