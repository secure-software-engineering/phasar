/*
 * LLVMInterMonotoneSolver.hh
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_HH_
#define SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_HH_

#include "../InterMonotoneProblem.hh"
#include "InterMonotoneSolver.hh"
#include <iostream>
#include <llvm/IR/Instruction.h>
using namespace std;

template <typename D, unsigned K, typename I>
class LLVMInterMonotoneSolver
    : public InterMonotoneSolver<const llvm::Instruction *, D,
                                 const llvm::Function *, const llvm::Value *,
                                 K, I> {
protected:
  bool DUMP_RESULTS;
  InterMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *,
                       const llvm::Value *, I> &IMP;

public:
  LLVMInterMonotoneSolver();
  virtual ~LLVMInterMonotoneSolver() = default;

  LLVMInterMonotoneSolver(
      InterMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *,
                           const llvm::Value *, I> &problem,
      bool dumpResults = false)
      : InterMonotoneSolver<const llvm::Instruction *, D,
                            const llvm::Function *, const llvm::Value *, K, I>(
            problem),
        DUMP_RESULTS(dumpResults), IMP(problem) {}

  virtual void solve() override {
    // do the solving of the analaysis problem
    InterMonotoneSolver<const llvm::Instruction *, D, const llvm::Function *,
                        const llvm::Value *, K, I>::solve();
    if (DUMP_RESULTS)
      dumpResults();
  }

  void dumpResults() {
    cout << "Monotone solver results:\n";
    // Iterate instructions
    for (auto &node : InterMonotoneSolver<const llvm::Instruction *, D,
                                          const llvm::Function *,
                                          const llvm::Value *, K, I>::Analysis) {
      cout << "Instruction: " << llvmIRToString(node.first) << "\n";
      // Iterate call-string - flow fact set pairs
      for (auto &flowfacts : node.second) {
        cout << "Context: ";
        // Print the elements of the call string
        for (auto cstring : flowfacts.first.getInternalCS()) {
          cout << ((llvm::isa<llvm::Function>(cstring))
          ? llvm::dyn_cast<llvm::Function>(cstring)->getName().str()
          : IMP.C_to_string(cstring)) << " * ";
        }
        cout << "\nFacts:\n";
        // Print the elements of the corresponding set of flow facts
        for (auto &flowfact : flowfacts.second) {
          cout << IMP.D_to_string(flowfact) << '\n';
        }
      }
      cout << "\n\n";
    }
  }
};

#endif /* SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_HH_ */
