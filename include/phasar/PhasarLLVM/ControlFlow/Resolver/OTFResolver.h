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

#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"

namespace llvm {
class Instruction;
class CallBase;
class Function;
class Type;
class Value;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMBasedICFG;
class LLVMTypeHierarchy;

class OTFResolver : public CHAResolver {
protected:
  LLVMBasedICFG &ICF;
  LLVMPointsToInfo &PT;

public:
  OTFResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH, LLVMBasedICFG &ICF,
              LLVMPointsToInfo &PT);

  ~OTFResolver() override = default;

  void preCall(const llvm::Instruction *Inst) override;

  void handlePossibleTargets(
      const llvm::CallBase *CallSite,
      std::set<const llvm::Function *> &CalleeTargets) override;

  void postCall(const llvm::Instruction *Inst) override;

  std::set<const llvm::Function *>
  resolveVirtualCall(const llvm::CallBase *CallSite) override;

  std::set<const llvm::Function *>
  resolveFunctionPointer(const llvm::CallBase *CallSite) override;

  static std::set<const llvm::Type *>
  getReachableTypes(const std::unordered_set<const llvm::Value *> &Values);

  static std::vector<std::pair<const llvm::Value *, const llvm::Value *>>
  getActualFormalPointerPairs(const llvm::CallBase *CallSite,
                              const llvm::Function *CalleeTarget);
};
} // namespace psr

#endif
