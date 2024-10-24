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
class DICompositeType;
} // namespace llvm

namespace psr {
class DIBasedTypeHierarchy;
class RTAResolver : public CHAResolver {
public:
  RTAResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
              const DIBasedTypeHierarchy *TH);

  ~RTAResolver() override = default;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return false;
  }

private:
  void resolveAllocatedCompositeTypes();

  std::vector<const llvm::DICompositeType *> AllocatedCompositeTypes;
};
} // namespace psr

#endif
