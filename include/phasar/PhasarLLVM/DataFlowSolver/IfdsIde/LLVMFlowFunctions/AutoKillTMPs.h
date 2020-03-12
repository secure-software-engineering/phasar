/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_AUTOKILLTMPS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_AUTOKILLTMPS_H_

#include <memory>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

#include "llvm/IR/Instructions.h"

namespace llvm {
class Value;
class Use;
class Instruction;
} // namespace llvm

namespace psr {

/**
 * A flow function that can be wrapped around another flow function
 * in order to kill unnecessary temporary values that are no longer
 * in use, but otherwise would be still propagated through the exploded
 * super-graph.
 * @brief Automatically kills temporary loads that are no longer in use.
 */
class AutoKillTMPs : public FlowFunction<const llvm::Value *> {
protected:
  std::shared_ptr<FlowFunction<const llvm::Value *>> delegate;
  const llvm::Instruction *inst;

public:
  AutoKillTMPs(std::shared_ptr<FlowFunction<const llvm::Value *>> ff,
               const llvm::Instruction *in);
  virtual ~AutoKillTMPs() = default;

  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *source) override;
};

} // namespace psr

#endif
