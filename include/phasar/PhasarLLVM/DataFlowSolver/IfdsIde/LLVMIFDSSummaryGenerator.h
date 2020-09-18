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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMIFDSSUMMARYGENERATOR_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMIFDSSUMMARYGENERATOR_H_

#include <set>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/DefaultIFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSummaryGenerator.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/LLVMIFDSSolver.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename I, typename ConcreteIFDSTabulationProblem>
class LLVMIFDSSummaryGenerator
    : public IFDSSummaryGenerator<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  I, ConcreteIFDSTabulationProblem,
                                  LLVMIFDSSolver<const llvm::Value *, I>> {
private:
  virtual std::vector<const llvm::Value *> getInputs() {
    std::vector<const llvm::Value *> inputs;
    // collect arguments
    for (auto &arg : this->toSummarize->args()) {
      inputs.push_back(&arg);
    }
    // collect global values
    auto globals = globalValuesUsedinFunction(this->toSummarize);
    inputs.insert(inputs.end(), globals.begin(), globals.end());
    return inputs;
  }

  virtual std::vector<bool>
  generateBitPattern(const std::vector<const llvm::Value *> &inputs,
                     const std::set<const llvm::Value *> &subset) {
    // initialize all bits to zero
    std::vector<bool> bitpattern(inputs.size(), 0);
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
} // namespace psr

#endif
