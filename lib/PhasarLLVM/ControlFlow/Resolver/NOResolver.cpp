/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Resolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include "phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h"

#include <set>

using namespace psr;

namespace psr {

NOResolver::NOResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP)
    : Resolver(IRDB, VTP) {}

auto NOResolver::resolveVirtualCall(const llvm::CallBase * /*CallSite*/)
    -> FunctionSetTy {
  return {};
}

auto NOResolver::resolveFunctionPointer(const llvm::CallBase * /*CallSite*/)
    -> FunctionSetTy {
  return {};
}

std::string NOResolver::str() const { return "NOResolver"; }

} // namespace psr
