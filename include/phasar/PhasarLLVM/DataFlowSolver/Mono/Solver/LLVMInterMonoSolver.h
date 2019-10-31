/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMInterMonoSolver.h
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTERMONOSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_LLVMINTERMONOSOLVER_H_

#include <iosfwd>
#include <memory>

#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>
#include <phasar/PhasarLLVM/Mono/Solver/InterMonoSolver.h>

namespace psr {

template <typename D, typename I, unsigned K>
class LLVMInterMonoSolver
    : public InterMonoSolver<const llvm::Instruction *, D,
                             const llvm::Function *, I, K> {
public:
  LLVMInterMonoSolver(InterMonoProblem<const llvm::Instruction *, D,
                                       const llvm::Function *, I> &problem)
      : InterMonoSolver<const llvm::Instruction *, D, const llvm::Function *, I,
                        K>(problem) {}

  ~LLVMInterMonoSolver() override = default;

  LLVMInterMonoSolver(const LLVMInterMonoSolver &copy) = delete;
  LLVMInterMonoSolver(LLVMInterMonoSolver &move) = delete;
  LLVMInterMonoSolver &operator=(const LLVMInterMonoSolver &copy) = delete;
  LLVMInterMonoSolver &operator=(LLVMInterMonoSolver &&move) = delete;

  void solve() override {
    // do the solving of the analaysis problem
    InterMonoSolver<const llvm::Instruction *, D, const llvm::Function *, I,
                    K>::solve();
    if (PhasarConfig::VariablesMap().count("emit-raw-results")) {
      dumpResults();
    }
  }

  /**
   * Dumps monotone solver results to the commandline.
   */
  void dumpResults() {
    std::cout << "======= DUMP LLVM-INTER-MONOTONE-SOLVER RESULTS =======\n";
    for (auto &entry :
         InterMonoSolver<const llvm::Instruction *, D, const llvm::Function *,
                         I, K>::Analysis) {
      std::cout << "Instruction:\n"
                << InterMonoSolver<const llvm::Instruction *, D,
                                   const llvm::Function *, I, K>::IMProblem
                       .NtoString(entry.first);
      std::cout << "\nFacts:\n";
      if (entry.second.empty()) {
        std::cout << "\tEMPTY\n";
      } else {
        for (auto &context : entry.second) {
          std::cout << context.first << '\n';
          if (context.second.empty()) {
            std::cout << "\tEMPTY\n";
          } else {
            for (auto &fact : context.second) {
              std::cout << InterMonoSolver<const llvm::Instruction *, D,
                                           const llvm::Function *, I,
                                           K>::IMProblem.DtoString(fact);
            }
          }
        }
      }
      std::cout << '\n';
    }
  };
};

} // namespace psr

#endif
