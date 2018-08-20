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
#include <string>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

namespace llvm {
class ImmutableCallSite;
}

namespace psr {
struct CHAResolver : public Resolver {
public:
  CHAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch);
  virtual ~CHAResolver() = default;

  virtual std::set<std::string>
  resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
};
} // namespace psr

#endif
