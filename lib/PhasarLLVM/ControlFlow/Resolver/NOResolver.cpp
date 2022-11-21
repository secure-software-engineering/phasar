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

#include <set>

#include "phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h"

using namespace psr;

namespace psr {

NOResolver::NOResolver(ProjectIRDB &IRDB) : Resolver(IRDB) {}

void NOResolver::preCall(const llvm::Instruction *Inst) {}

void NOResolver::handlePossibleTargets(const llvm::CallBase *CallSite,
                                       FunctionSetTy &PossibleTargets) {}

void NOResolver::postCall(const llvm::Instruction *Inst) {}

auto NOResolver::resolveVirtualCall(const llvm::CallBase * /*CallSite*/)
    -> FunctionSetTy {
  return {};
}

auto NOResolver::resolveFunctionPointer(const llvm::CallBase * /*CallSite*/)
    -> FunctionSetTy {
  return {};
}

void NOResolver::otherInst(const llvm::Instruction *Inst) {}

std::string NOResolver::str() const { return "NOResolver"; }

} // namespace psr
