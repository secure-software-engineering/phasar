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

#ifndef SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_H_
#define SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_H_

#include <iostream>
#include <llvm/IR/Instruction.h>

#include "../IntraMonotoneProblem.h"
#include "IntraMonotoneSolver.h"
using namespace std;

namespace psr{

template <typename D, typename C>
class LLVMIntraMonotoneSolver
    : public IntraMonotoneSolver<const llvm::Instruction *, D,
                                 const llvm::Function *, C> {
protected:
  bool DUMP_RESULTS;
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
    cout << "Monotone solver results:\n"
            "------------------------\n";
    for (auto &entry :
         IntraMonotoneSolver<const llvm::Instruction *, D,
                             const llvm::Function *, C>::Analysis) {
      cout << "Instruction:\n";
      entry.first->print(llvm::outs());
      cout << "Facts:\n";
      if (entry.second.empty()) {
        cout << "\tEMPTY\n";
      } else {
        for (auto fact : entry.second) {
          cout << IMP.DtoString(fact) << '\n';
        }
      }
      cout << "\n\n";
    }
  }
};

}//namespace psr

#endif /* SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_HH_ */
