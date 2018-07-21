/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * RTAResolver.h
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#pragma once

#include <string>
#include <set>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

namespace llvm {
  class ImmutableCallSite;
  class StructType;
  class Function;
}

namespace psr {
  struct RTAResolver : virtual public CHAResolver {
  protected:
    std::set<const llvm::StructType*> unsound_types;

  public:
    RTAResolver(ProjectIRDB &irdb, const LLVMTypeHierarchy &ch);

    virtual void firstFunction(const llvm::Function* F) override;
    virtual std::set<std::string> resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
  };
} // namespace psr
