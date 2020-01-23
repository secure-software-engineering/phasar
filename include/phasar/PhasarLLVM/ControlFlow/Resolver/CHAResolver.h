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

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_CHARESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_CHARESOLVER_H_

#include <set>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

namespace llvm {
class ImmutableCallSite;
class Function;
} // namespace llvm

namespace psr {
class CHAResolver : public Resolver {
public:
  CHAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  ~CHAResolver() override = default;

  std::set<const llvm::Function *>
  resolveVirtualCall(llvm::ImmutableCallSite CS) override;
};
} // namespace psr

#endif
