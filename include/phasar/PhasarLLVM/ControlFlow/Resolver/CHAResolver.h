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

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"

namespace llvm {
class CallBase;
class Function;
} // namespace llvm

namespace psr {
class CHAResolver : public Resolver {
public:
  CHAResolver(LLVMProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  ~CHAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;
};
} // namespace psr

#endif
