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

#include <llvm/IR/CallSite.h>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h>

using namespace psr;

namespace psr {

NOResolver::NOResolver(ProjectIRDB &IRDB) : Resolver(IRDB) {}

void NOResolver::preCall(const llvm::Instruction *Inst) {}

void NOResolver::handlePossibleTargets(
    llvm::ImmutableCallSite CS,
    std::set<const llvm::Function *> &PossibleTargets) {}

void NOResolver::postCall(const llvm::Instruction *Inst) {}

std::set<const llvm::Function *>
NOResolver::resolveVirtualCall(llvm::ImmutableCallSite CS) {
  return {};
}

std::set<const llvm::Function *>
NOResolver::resolveFunctionPointer(llvm::ImmutableCallSite CS) {
  return {};
}

void NOResolver::otherInst(const llvm::Instruction *Inst) {}

} // namespace psr
