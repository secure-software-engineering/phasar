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

using namespace psr;

NOResolver::NOResolver(const LLVMProjectIRDB *IRDB,
                       const LLVMVFTableProvider *VTP)
    : Resolver(IRDB, VTP) {}

void NOResolver::resolveVirtualCall(FunctionSetTy & /*PossibleTargets*/,
                                    const llvm::CallBase * /*CallSite*/) {}

void NOResolver::resolveFunctionPointer(FunctionSetTy & /*PossibleTargets*/,
                                        const llvm::CallBase * /*CallSite*/) {}

std::string NOResolver::str() const { return "NOResolver"; }
