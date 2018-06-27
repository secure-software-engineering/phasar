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

#pragma once

#include "../InterMonotoneProblem.h"
#include "InterMonotoneGeneralizedSolver.h"
#include "../Contexts/CallString.h"

#include <iostream>

#include <llvm/IR/Instruction.h>

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
//     std::cout << "Monotone solver results:\n";
//     // Iterate instructions
//     for (auto &node :
//          InterMonotoneGeneralizedSolver<const llvm::Instruction *, D,
//                              const llvm::Function *, const llvm::Value *,
//                              I, CallString<const llvm::Instruction *, D, K>>::Analysis) {
//       std::cout << "Instruction: " << llvmIRToString(node.first) << " in "
//            << node.first->getFunction()->getName().str() << "\n";
//       // Iterate call-std::string - flow fact std::set pairs
//       for (auto &flowfacts : node.second) {
//         std::cout << "Context: ";
//         // Print the elements of the call std::string
//         for (auto cstd::string : flowfacts.first.getInternalCS()) {
//           std::cout
//               << ((llvm::isa<llvm::Function>(cstd::string))
//                       ? llvm::dyn_cast<llvm::Function>(cstd::string)->getName().str()
//                       : IMProblem.CtoString(cstd::string))
//               << " * ";
//         }
//         std::cout << "\nFacts:\n";
//         // Print the elements of the corresponding std::set of flow facts
//         for (auto &flowfact : flowfacts.second) {
//           std::cout << IMProblem.DtoString(flowfact) << '\n';
//         }
//       }
//       std::cout << "\n\n";
//     }
//   }
// };

} // namespace psr
