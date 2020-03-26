/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_MAPFACTSTOCALLER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_MAPFACTSTOCALLER_H_

#include <functional>
#include <vector>

#include "llvm/IR/CallSite.h" // llvm::ImmutableCallSite

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

namespace llvm {
class Function;
class Value;
class Instruction;
} // namespace llvm

namespace psr {

/**
 * Predicates can be used to specifiy additonal requirements for mapping
 * actual parameters into formal parameters and the return value.
 * @note Currently, the return value predicate only allows checks regarding
 * the callee method.
 * @brief Generates all valid actual parameters and the return value in the
 * caller context.
 */
class MapFactsToCaller : public FlowFunction<const llvm::Value *> {
private:
  llvm::ImmutableCallSite callSite;
  const llvm::Function *calleeFun;
  const llvm::ReturnInst *exitStmt;
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;
  std::function<bool(const llvm::Value *)> paramPredicate;
  std::function<bool(const llvm::Function *)> returnPredicate;

public:
  MapFactsToCaller(
      llvm::ImmutableCallSite cs, const llvm::Function *calleeFun,
      const llvm::Instruction *exitstmt,
      std::function<bool(const llvm::Value *)> paramPredicate =
          [](const llvm::Value *) { return true; },
      std::function<bool(const llvm::Function *)> returnPredicate =
          [](const llvm::Function *) { return true; });
  virtual ~MapFactsToCaller() = default;

  std::set<const llvm::Value *> computeTargets(const llvm::Value *source);
};

} // namespace psr

#endif
