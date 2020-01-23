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
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>

namespace llvm {
class Instruction;
class ImmutableCallSite;
class Function;
} // namespace llvm

namespace psr {
class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;
class PointsToGraph;

class OTFResolver : public CHAResolver {
protected:
  LLVMPointsToInfo &PT;
  PointsToGraph &WholeModulePTG;
  std::vector<const llvm::Instruction *> CallStack;

public:
  OTFResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH, LLVMPointsToInfo &PT,
              PointsToGraph &WholeModulePTG);

  ~OTFResolver() override = default;

  void preCall(const llvm::Instruction *Inst) override;

  void handlePossibleTargets(
      llvm::ImmutableCallSite CS,
      std::set<const llvm::Function *> &possible_targets) override;

  void postCall(const llvm::Instruction *Inst) override;

  std::set<const llvm::Function *>
  resolveVirtualCall(llvm::ImmutableCallSite CS) override;

  std::set<const llvm::Function *>
  resolveFunctionPointer(llvm::ImmutableCallSite CS) override;
};
} // namespace psr

#endif
