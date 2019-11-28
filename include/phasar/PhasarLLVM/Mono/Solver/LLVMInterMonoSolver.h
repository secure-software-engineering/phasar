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

#include <algorithm>
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
    std::cout
        << "\n**************************************************************\n"
        << "*             Raw LLVM-Inter-Mono-Solver results             *\n"
        << "**************************************************************\n";
    typedef std::pair<
        const llvm::Instruction *,
        std::unordered_map<CallStringCTX<const llvm::Instruction *, K>,
                           BitVectorSet<D>>>
        NtoDTy;
    std::vector<NtoDTy> sortedRes(
        InterMonoSolver<const llvm::Instruction *, D, const llvm::Function *, I,
                        K>::Analysis.begin(),
        InterMonoSolver<const llvm::Instruction *, D, const llvm::Function *, I,
                        K>::Analysis.end());
    llvmValueIDLess llvmIDLess;
    std::sort(sortedRes.begin(), sortedRes.end(),
              [&llvmIDLess](NtoDTy a, NtoDTy b) {
                return llvmIDLess(a.first, b.first);
              });
    for (auto &entry : sortedRes) {
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
            for (auto &fact : context.second.getAsSet()) {
              std::cout << "D: "
                        << InterMonoSolver<const llvm::Instruction *, D,
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
