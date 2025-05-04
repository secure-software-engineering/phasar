/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_NORESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_NORESOLVER_H_

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"

namespace llvm {
class CallBase;
} // namespace llvm

namespace psr {

class NOResolver final : public Resolver {
public:
  NOResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP);

  ~NOResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  FunctionSetTy resolveFunctionPointer(const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return false;
  }
};
} // namespace psr

#endif
