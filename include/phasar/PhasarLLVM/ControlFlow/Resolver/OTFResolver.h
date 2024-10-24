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

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace llvm {
class CallBase;
class Function;
class Type;
class Value;
} // namespace llvm

namespace psr {

class DIBasedTypeHierarchy;

class OTFResolver : public Resolver {
public:
  OTFResolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
              LLVMAliasInfoRef PT);

  ~OTFResolver() override = default;

  void handlePossibleTargets(const llvm::CallBase *CallSite,
                             FunctionSetTy &CalleeTargets) override;

  FunctionSetTy resolveVirtualCall(const llvm::CallBase *CallSite) override;

  FunctionSetTy resolveFunctionPointer(const llvm::CallBase *CallSite) override;

  static std::set<const llvm::Type *>
  getReachableTypes(const LLVMAliasInfo::AliasSetTy &Values);

  static std::vector<std::pair<const llvm::Value *, const llvm::Value *>>
  getActualFormalPointerPairs(const llvm::CallBase *CallSite,
                              const llvm::Function *CalleeTarget);

  [[nodiscard]] std::string str() const override;

  [[nodiscard]] bool
  mutatesHelperAnalysisInformation() const noexcept override {
    return true;
  }

protected:
  LLVMAliasInfoRef PT;
};
} // namespace psr

#endif
