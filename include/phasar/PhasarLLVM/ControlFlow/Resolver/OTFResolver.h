/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * OTFResolver.h
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_OTFRESOLVER_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_RESOLVER_OTFRESOLVER_H_

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

namespace psr {

/// \brief A resolver that uses alias information to resolve indirect and
/// virtual calls
class OTFResolver : public Resolver {
public:
  OTFResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
              LLVMAliasInfoRef PT);

  ~OTFResolver() override = default;

  void handlePossibleTargets(const llvm::CallBase *CallSite,
                             FunctionSetTy &CalleeTargets) override;

  void resolveVirtualCall(FunctionSetTy &PossibleTargets,
                          const llvm::CallBase *CallSite) override;

  void resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                              const llvm::CallBase *CallSite) override;

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return !PT.isInterProcedural();
  }

protected:
  LLVMAliasInfoRef PT;
};
} // namespace psr

#endif
