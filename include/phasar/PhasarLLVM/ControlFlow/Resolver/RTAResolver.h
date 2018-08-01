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

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_RTARESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_RTARESOLVER_H_

#include <set>
#include <string>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>

namespace llvm {
class ImmutableCallSite;
class StructType;
class Function;
} // namespace llvm

namespace psr {
struct RTAResolver : public CHAResolver {
protected:
  std::set<const llvm::StructType *> unsound_types;

public:
  RTAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch);
  virtual ~RTAResolver() = default;

  virtual void firstFunction(const llvm::Function *F) override;
  virtual std::set<std::string>
  resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
};
} // namespace psr

#endif
