/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MapFactsToCallee.h
 *
 *  Created on: 27.04.2018
 *      Author: rleer
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLEE_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLEE_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/Utils/LLVMShorthands.h>
namespace psr{

/**
 * @brief Generates all valid formal parameter in the callee context.
 */
class MapFactsToCallee : public FlowFunction<const llvm::Value *> {
private:
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;

public:
  MapFactsToCallee(llvm::ImmutableCallSite callSite,
                   const llvm::Function *destMthd) {
    // Set up the actual parameters
    for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
      actuals.push_back(callSite.getArgOperand(idx));
    }
    // Set up the formal parameters
    for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
      formals.push_back(getNthFunctionArgument(destMthd, idx));
    }
  }
  virtual ~MapFactsToCallee() = default;
  std::set<const llvm::Value *> computeTargets(const llvm::Value *source) {
    if (!isLLVMZeroValue(source)) {
      std::set<const llvm::Value *> res;
      // Map actual parameter into corresponding formal parameter.
      for (unsigned idx = 0; idx < actuals.size(); ++idx) {
        if (source == actuals[idx]) {
          res.insert(formals[idx]); // corresponding formal
        }
      }
      return res;
    } else {
      return {source};
    }
  }
};

}//namespace psr

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_MAPFACTSTOCALLEE_H_ */
