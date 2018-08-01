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
class PointsToGraph;

struct OTFResolver : public CHAResolver {
protected:
  PointsToGraph &WholeModulePTG;
  std::vector<const llvm::Instruction *> CallStack;

public:
  OTFResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch,
              PointsToGraph &wholemodulePTG);
  virtual ~OTFResolver() = default;

  virtual void preCall(const llvm::Instruction *Inst) override;
  virtual void TreatPossibleTarget(
      const llvm::ImmutableCallSite &CS,
      std::set<const llvm::Function *> &possible_targets) override;
  virtual void postCall(const llvm::Instruction *Inst) override;
  virtual void OtherInst(const llvm::Instruction *Inst) override;
  virtual std::set<std::string>
  resolveVirtualCall(const llvm::ImmutableCallSite &CS) override;
};
} // namespace psr

#endif
