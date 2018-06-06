/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_

#include <functional>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/Utils/LLVMShorthands.h>
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
  const llvm::Function *calleeMthd;
  const llvm::ReturnInst *exitStmt;
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;
  std::function<bool(const llvm::Value *)> paramPredicate;
  std::function<bool(const llvm::Function *)> returnPredicate;

public:
  MapFactsToCaller(llvm::ImmutableCallSite cs, const llvm::Function *calleeMthd,
                   const llvm::Instruction *exitstmt,
                   std::function<bool(const llvm::Value *)> paramPredicate =
                       [](const llvm::Value *) { return true; },
                   std::function<bool(const llvm::Function *)> returnPredicate =
                       [](const llvm::Function *) { return true; })
      : callSite(cs), calleeMthd(calleeMthd),
        exitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)),
        paramPredicate(paramPredicate), returnPredicate(returnPredicate) {
    // Set up the actual parameters
    for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
      actuals.push_back(callSite.getArgOperand(idx));
    }
    // Set up the formal parameters
    for (unsigned idx = 0; idx < calleeMthd->arg_size(); ++idx) {
      formals.push_back(getNthFunctionArgument(calleeMthd, idx));
    }
  }
  virtual ~MapFactsToCaller() = default;
  std::set<const llvm::Value *> computeTargets(const llvm::Value *source) {
    if (!isLLVMZeroValue(source)) {
      std::set<const llvm::Value *> res;
      // Map formal parameter into corresponding actual parameter.
      for (unsigned idx = 0; idx < formals.size(); ++idx) {
        if (source == formals[idx] && paramPredicate(formals[idx])) {
          res.insert(actuals[idx]); // corresponding actual
        }
      }
      // Collect return value facts
      if (source == exitStmt->getReturnValue() && returnPredicate(calleeMthd)) {
        res.insert(callSite.getInstruction());
      }
      return res;
    } else {
      return {source};
    }
  }
};

} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_ */
