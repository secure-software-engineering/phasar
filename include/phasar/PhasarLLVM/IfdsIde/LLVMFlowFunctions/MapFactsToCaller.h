/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MapFactsToCaller.h
 *
 *  Created on: 27.04.2018
 *      Author: rleer
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/Utils/LLVMShorthands.h>

/**
 * @brief Generates all valid actual parameters and the return value in the
 * caller context.
 */
class MapFactsToCaller : public FlowFunction<const llvm::Value *> {
private:
  llvm::ImmutableCallSite callSite;
  const llvm::ReturnInst *exitStmt;
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;

public:
  MapFactsToCaller(llvm::ImmutableCallSite cs, const llvm::Function *calleeMthd,
                   const llvm::Instruction *exitstmt)
      : callSite(cs), exitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)) {
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
        if (source == formals[idx]) {
          res.insert(actuals[idx]); // corresponding actual
        }
      }
      // Collect return value facts
      if (source == exitStmt->getReturnValue()) {
        res.insert(callSite.getInstruction());
      }
      return res;
    } else {
      return {source};
    }
  }
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLER_H_ */
