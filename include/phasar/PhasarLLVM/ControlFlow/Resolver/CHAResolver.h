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
#include "phasar/Utils/MaybeUniquePtr.h"

namespace llvm {
class CallBase;
class Function;
} // namespace llvm

namespace psr {
class DIBasedTypeHierarchy;
class CHAResolver : public Resolver {
public:
  CHAResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
              const DIBasedTypeHierarchy *TH);

  // Deleting an incomplete type (LLVMTypeHierarchy) is UB, so instantiate the
  // dtor in CHAResolver.cpp
  ~CHAResolver() override;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  [[nodiscard]] bool isIndependent() const noexcept override { return true; }

  [[nodiscard]] std::string str() const override;

protected:
  MaybeUniquePtr<const DIBasedTypeHierarchy, true> TH;
};
} // namespace psr

#endif
