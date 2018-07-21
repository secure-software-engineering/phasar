/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CHAResolver.h
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
}

namespace psr {
  struct CHAResolver : virtual public Resolver {
  public:
    CHAResolver(ProjectIRDB &irdb, const LLVMTypeHierarchy &ch);

    virtual std::set<std::string> resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
  };
} // namespace psr
