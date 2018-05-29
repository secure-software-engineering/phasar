/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSummaryGenerator.h
 *
 *  Created on: 03.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_H_
#define SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_H_

#include "../../db/ProjectIRDB.h"
#include "../../lib/LLVMShorthands.h"
#include "../../utils/utils.h"
#include "../control_flow/ICFG.h"
#include "../control_flow/LLVMBasedICFG.h"
#include "../ifds_ide/FlowFunction.h"
#include "../ifds_ide/flow_func/GenAll.h"
#include "DefaultIFDSTabulationProblem.h"
#include "IFDSTabulationProblem.h"
#include "solver/IFDSSummaryGenerator.h"
#include "solver/LLVMIFDSSolver.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>
using namespace std;
namespace psr{

template <typename I, typename ConcreteIFDSTabulationProblem>
class LLVMIFDSSummaryGenerator
    : public IFDSSummaryGenerator<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  I, ConcreteIFDSTabulationProblem,
                                  LLVMIFDSSolver<const llvm::Value *, I>> {
private:
  virtual vector<const llvm::Value *> getInputs() {
    vector<const llvm::Value *> inputs;
    // collect arguments
    for (auto &arg : this->toSummarize->args()) {
      inputs.push_back(&arg);
    }
    // collect global values
    auto globals = globalValuesUsedinFunction(this->toSummarize);
    inputs.insert(inputs.end(), globals.begin(), globals.end());
    return inputs;
  }

  virtual vector<bool>
  generateBitPattern(const vector<const llvm::Value *> &inputs,
                     const set<const llvm::Value *> &subset) {
    // initialize all bits to zero
    vector<bool> bitpattern(inputs.size(), 0);
    if (subset.empty()) {
      return bitpattern;
    }
    for (auto elem : subset) {
      for (size_t i = 0; i < inputs.size(); ++i) {
        if (elem == inputs[i]) {
          bitpattern[i] = 1;
        }
      }
    }
    return bitpattern;
  }

public:
  LLVMIFDSSummaryGenerator(const llvm::Function *F, I icfg,
                           SummaryGenerationStrategy S)
      : IFDSSummaryGenerator<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Function *, I,
                             ConcreteIFDSTabulationProblem,
                             LLVMIFDSSolver<const llvm::Value *, I>>(F, icfg,
                                                                     S) {}

  virtual ~LLVMIFDSSummaryGenerator() = default;
};
}//namespace psr

#endif /* SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_HH_ */
