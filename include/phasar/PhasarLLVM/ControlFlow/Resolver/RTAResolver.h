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

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"

#include <vector>

namespace llvm {
class CallBase;
class StructType;
class Function;
class StructType;
} // namespace llvm

namespace psr {
class RTAResolver : public CHAResolver {
public:
  RTAResolver(LLVMProjectIRDB &IRDB, LLVMTypeHierarchy &TH);

  ~RTAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

private:
  void resolveAllocatedStructTypes();

  std::vector<const llvm::StructType *> AllocatedStructTypes;
};
} // namespace psr

#endif
