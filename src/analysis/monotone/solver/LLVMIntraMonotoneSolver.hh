/*
 * LLVMMonotoneSolver.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_HH_
#define SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_HH_

#include <iostream>
#include <llvm/IR/Instruction.h>

#include "../IntraMonotoneProblem.hh"
#include "IntraMonotoneSolver.hh"
using namespace std;

template <typename D, typename C>
class LLVMIntraMonotoneSolver
    : public IntraMonotoneSolver<const llvm::Instruction *, D,
                                 const llvm::Function *, C> {
protected:
  bool DUMP_RESULTS;

public:
  LLVMIntraMonotoneSolver();
  virtual ~LLVMIntraMonotoneSolver() = default;

  LLVMIntraMonotoneSolver(
      IntraMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *,
                           C> &problem,
      bool dumpResults = false)
      : IntraMonotoneSolver<const llvm::Instruction *, D,
                            const llvm::Function *, C>(problem),
        DUMP_RESULTS(dumpResults) {}

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
      entry.first->dump();
      cout << "Facts:\n";
      if (entry.second.empty()) {
        cout << "\tEMPTY\n";
      } else {
        for (auto fact : entry.second) {
          fact->dump();
        }
      }
      cout << "\n\n";
    }
  }
};

#endif /* SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTRAMONOTONESOLVER_HH_ */
