/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMInterMonotoneSolver.h
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_H_
#define SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_H_

#include "../InterMonotoneProblem.h"
#include "InterMonotoneGeneralizedSolver.h"
#include "../Contexts/CallString.h"

#include <iostream>

#include <llvm/IR/Instruction.h>


// using namespace std;

namespace psr {
//
// template <typename D, unsigned K, typename I>
// class LLVMInterMonotoneSolver
//     : public InterMonotoneGeneralizedSolver<const llvm::Instruction *, D,
//                                  const llvm::Function *, const llvm::Value *, I, CallString<const llvm::Instruction *, D, K>> {
// protected:
//   bool DUMP_RESULTS;
//
// public:
//   LLVMInterMonotoneSolver();
//   virtual ~LLVMInterMonotoneSolver() = default;
//
//   LLVMInterMonotoneSolver(
//       InterMonotoneProblem<const llvm::Instruction *, D, const llvm::Function *,
//                            const llvm::Value *, I> &problem,
//       bool dumpResults = false)
//       : InterMonotoneGeneralizedSolver<const llvm::Instruction *, D,
//                             const llvm::Function *, const llvm::Value *, I, CallString<const llvm::Instruction *, D, K>>(
//             problem, CallString<const llvm::Instruction *, D, K>(), problem.ICFG.getMethod("main")),
//         DUMP_RESULTS(dumpResults) {}
//
//   virtual void solve() override {
//     // do the solving of the analaysis problem
//     InterMonotoneGeneralizedSolver<const llvm::Instruction *, D, const llvm::Function *,
//                         const llvm::Value *, I, CallString<const llvm::Instruction *, D, K>>::solve();
//     if (DUMP_RESULTS)
//       dumpResults();
//   }
//
//   void dumpResults() {
//     cout << "Monotone solver results:\n";
//     // Iterate instructions
//     for (auto &node :
//          InterMonotoneGeneralizedSolver<const llvm::Instruction *, D,
//                              const llvm::Function *, const llvm::Value *,
//                              I, CallString<const llvm::Instruction *, D, K>>::Analysis) {
//       cout << "Instruction: " << llvmIRToString(node.first) << " in "
//            << node.first->getFunction()->getName().str() << "\n";
//       // Iterate call-string - flow fact set pairs
//       for (auto &flowfacts : node.second) {
//         cout << "Context: ";
//         // Print the elements of the call string
//         for (auto cstring : flowfacts.first.getInternalCS()) {
//           cout
//               << ((llvm::isa<llvm::Function>(cstring))
//                       ? llvm::dyn_cast<llvm::Function>(cstring)->getName().str()
//                       : IMProblem.CtoString(cstring))
//               << " * ";
//         }
//         cout << "\nFacts:\n";
//         // Print the elements of the corresponding set of flow facts
//         for (auto &flowfact : flowfacts.second) {
//           cout << IMProblem.DtoString(flowfact) << '\n';
//         }
//       }
//       cout << "\n\n";
//     }
//   }
// };

} // namespace psr

#endif /* SRC_ANALYSIS_MONOTONE_SOLVER_LLVMINTERMONOTONESOLVER_HH_ */
