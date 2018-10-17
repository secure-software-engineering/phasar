/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIntraMonoSolver.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTRAMONOSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTRAMONOSOLVER_H_

#include <iostream> //  std::cout, please remove it

#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>
#include <phasar/PhasarLLVM/Mono/Solver/IntraMonoSolver.h>

namespace psr {

template <typename D, typename C>
class LLVMIntraMonoSolver : public IntraMonoSolver<const llvm::Instruction *, D,
                                                   const llvm::Function *, C> {
protected:
  bool DUMP_RESULTS;
  // Duplicate of the IMProblem of IntraMonoSolver ...
  IntraMonoProblem<const llvm::Instruction *, D, const llvm::Function *, C>
      &IMP;

public:
  LLVMIntraMonoSolver();
  virtual ~LLVMIntraMonoSolver() = default;

  LLVMIntraMonoSolver(IntraMonoProblem<const llvm::Instruction *, D,
                                       const llvm::Function *, C> &problem,
                      bool dumpResults = false)
      : IntraMonoSolver<const llvm::Instruction *, D, const llvm::Function *,
                        C>(problem),
        DUMP_RESULTS(dumpResults), IMP(problem) {}

  virtual void solve() override {
    // do the solving of the analaysis problem
    IntraMonoSolver<const llvm::Instruction *, D, const llvm::Function *,
                    C>::solve();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    std::cout << "LLVM-Intra-Monotone solver results:\n"
                 "-----------------------------------\n";
    for (auto &entry : IntraMonoSolver<const llvm::Instruction *, D,
                                       const llvm::Function *, C>::Analysis) {
      std::cout << "Instruction:\n" << IMP.NtoString(entry.first);
      std::cout << "\nFacts:\n";
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
