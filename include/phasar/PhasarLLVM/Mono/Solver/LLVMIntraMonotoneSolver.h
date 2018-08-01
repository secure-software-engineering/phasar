/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMMonotoneSolver.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTRAMONOTONESOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTRAMONOTONESOLVER_H_

#include <iostream> //  std::cout, please remove it

#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/Mono/IntraMonotoneProblem.h>
#include <phasar/PhasarLLVM/Mono/Solver/IntraMonotoneSolver.h>

namespace psr {

template <typename D, typename C>
class LLVMIntraMonotoneSolver
    : public IntraMonotoneSolver<const llvm::Instruction *, D,
                                 const llvm::Function *, C> {
protected:
  bool DUMP_RESULTS;
  // Duplicate of the IMProblem of IntraMonotoneSolver ...
  IntraMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *, C>
      &IMP;

public:
  LLVMIntraMonotoneSolver();
  virtual ~LLVMIntraMonotoneSolver() = default;

  LLVMIntraMonotoneSolver(
      IntraMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *,
                           C> &problem,
      bool dumpResults = false)
      : IntraMonotoneSolver<const llvm::Instruction *, D,
                            const llvm::Function *, C>(problem),
        DUMP_RESULTS(dumpResults), IMP(problem) {}

  virtual void solve() override {
    // do the solving of the analaysis problem
    IntraMonotoneSolver<const llvm::Instruction *, D, const llvm::Function *,
                        C>::solve();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    std::cout << "Monotone solver results:\n"
            "------------------------\n";
    for (auto &entry :
         IntraMonotoneSolver<const llvm::Instruction *, D,
                             const llvm::Function *, C>::Analysis) {
      std::cout << "Instruction:\n";
      entry.first->print(llvm::outs());
      std::cout << "Facts:\n";
      if (entry.second.empty()) {
        std::cout << "\tEMPTY\n";
      } else {
        for (auto fact : entry.second) {
          std::cout << IMP.DtoString(fact) << '\n';
        }
      }
      std::cout << "\n\n";
    }
  }
};

} // namespace psr

#endif
