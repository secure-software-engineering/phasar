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

#include <iostream>
#include <memory>

#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ValueBasedContext.h>
#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>
#include <phasar/PhasarLLVM/Mono/Solver/InterMonoGeneralizedSolver.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace psr {

struct LLVMOrderingById {
  using edge_t =
      std::pair<const llvm::Instruction *, const llvm::Instruction *>;
  bool lessInstruction(const llvm::Instruction *lhs,
                       const llvm::Instruction *rhs) const {
    return std::stol(getMetaDataID(lhs)) < std::stol(getMetaDataID(rhs));
  }

  bool operator()(const edge_t &lhs, const edge_t &rhs) const {
    return lessInstruction(lhs.first, rhs.first) ||
           (!lessInstruction(rhs.first, lhs.first) &&
            lessInstruction(lhs.second, rhs.second));
  }
};

template <typename D, class Context, class Ordering = LLVMOrderingById>
class LLVMInterMonoSolver
    : public InterMonoGeneralizedSolver<
          InterMonoProblem<const llvm::Instruction *, D, const llvm::Function *,
                           LLVMBasedICFG &>,
          Context, Ordering> {
public:
  using IMSBase_t = InterMonoGeneralizedSolver<
      InterMonoProblem<const llvm::Instruction *, D, const llvm::Function *,
                       LLVMBasedICFG &>,
      Context, Ordering>;

protected:
  bool DUMP_RESULTS;

public:
  LLVMInterMonoSolver(typename IMSBase_t::IMP_t &IMP, Context &_Context,
                      const llvm::Function *Method, bool dump = false)
      : IMSBase_t(IMP, _Context, Method), DUMP_RESULTS(dump) {}
  virtual ~LLVMInterMonoSolver() = default;

  LLVMInterMonoSolver(const LLVMInterMonoSolver &copy) = delete;
  LLVMInterMonoSolver(LLVMInterMonoSolver &move) = delete;
  LLVMInterMonoSolver &operator=(const LLVMInterMonoSolver &copy) = delete;
  LLVMInterMonoSolver &operator=(LLVMInterMonoSolver &&move) = delete;

  /**
   * Dumps monotone solver results to the commandline.
   */
  void dumpResults() {
    std::cout << "======= DUMP LLVM-INTER-MONOTONE-SOLVER RESULTS =======\n";
    // Iterate instructions
    for (auto &Node : IMSBase_t::Analysis) {
      std::cout << "------- Mono Start Result Record -------\n";
      std::cout << "F: " << Node.first->getFunction()->getName().str()
                << "   N: " << this->IMProblem.NtoString(Node.first) << '\n';
      // Iterate call-string - flow fact set pairs
      for (auto &ContextFactPair : Node.second) {
        std::cout << ContextFactPair.first;
        std::cout << "\nDomain:\n";
        // Print the elements of the corresponding set of flow facts
        std::cout << this->IMProblem.DtoString(ContextFactPair.second) << '\n';
      }
    }
  };
};

template <typename IMP_t, typename Context_t,
          typename Ordering_t = LLVMOrderingById>
auto make_LLVMBasedIMS(IMP_t &IMP, Context_t &Context,
                       typename IMP_t::Method_t Method, bool dump = false)
    -> std::unique_ptr<
        LLVMInterMonoSolver<typename IMP_t::Domain_t, Context_t, Ordering_t>> {
  using ptr_t =
      LLVMInterMonoSolver<typename IMP_t::Domain_t, Context_t, Ordering_t>;
  return std::make_unique<ptr_t>(IMP, Context, Method, dump);
}

// template <typename V, unsigned K, class Ordering = LLVMOrderingById>
// class LLVMInterMonoCallStringSolver
//   : public LLVMInterMonoSolver<V, CallString<const llvm::Instruction *,
//   V, K>> { public:
//     LLVMInterMonoCallStringSolver();
//     virtual ~LLVMInterMonoCallStringSolver() = default;
//
//     LLVMInterMonoCallStringSolver(const
//     LLVMInterMonoCallStringSolver& copy) = delete;
//     LLVMInterMonoCallStringSolver(LLVMInterMonoCallStringSolver&
//     move) = delete; LLVMInterMonoCallStringSolver& operator=(const
//     LLVMInterMonoCallStringSolver& copy) = delete;
//     LLVMInterMonoCallStringSolver&
//     operator=(LLVMInterMonoCallStringSolver&& move) = delete;
// };
//
// template <typename V, class Ordering = LLVMOrderingById>
// class LLVMInterMonoValueBasedContextSolver
//   : public LLVMInterMonoSolver<V, ValueBasedContext<const
//   llvm::Instruction *, V>> { public:
//     LLVMInterMonoValueBasedContextSolver() = default;
//     virtual ~LLVMInterMonoValueBasedContextSolver() = default;
//
//     LLVMInterMonoValueBasedContextSolver(const
//     LLVMInterMonoValueBasedContextSolver& copy) = delete;
//     LLVMInterMonoValueBasedContextSolver(LLVMInterMonoValueBasedContextSolver&
//     move) = delete; LLVMInterMonoValueBasedContextSolver& operator=(const
//     LLVMInterMonoValueBasedContextSolver& copy) = delete;
//     LLVMInterMonoValueBasedContextSolver&
//     operator=(LLVMInterMonoValueBasedContextSolver&& move) = delete;
// };

//
// template <typename D, unsigned K, typename I>
// class LLVMInterMonoSolver
//     : public InterMonoGeneralizedSolver<const llvm::Instruction *, D,
//                                  const llvm::Function *, const llvm::Value *,
//                                  I, CallString<const llvm::Instruction *, D,
//                                  K>> {
// protected:
//   bool DUMP_RESULTS;
//
// public:
//   LLVMInterMonoSolver();
//   virtual ~LLVMInterMonoSolver() = default;
//
//   LLVMInterMonoSolver(
//       InterMonoProblem<const llvm::Instruction *, D, const llvm::Function
//       *,
//                            const llvm::Value *, I> &problem,
//       bool dumpResults = false)
//       : InterMonoGeneralizedSolver<const llvm::Instruction *, D,
//                             const llvm::Function *, const llvm::Value *, I,
//                             CallString<const llvm::Instruction *, D, K>>(
//             problem, CallString<const llvm::Instruction *, D, K>(),
//             problem.ICFG.getMethod("main")),
//         DUMP_RESULTS(dumpResults) {}
//
//   virtual void solve() override {
//     // do the solving of the analaysis problem
//     InterMonoGeneralizedSolver<const llvm::Instruction *, D, const
//     llvm::Function *,
//                         const llvm::Value *, I, CallString<const
//                         llvm::Instruction *, D, K>>::solve();
//     if (DUMP_RESULTS)
//       dumpResults();
//   }
//
//   void dumpResults() {
//     std::cout << "Monotone solver results:\n";
//     // Iterate instructions
//     for (auto &node :
//          InterMonoGeneralizedSolver<const llvm::Instruction *, D,
//                              const llvm::Function *, const llvm::Value *,
//                              I, CallString<const llvm::Instruction *, D,
//                              K>>::Analysis) {
//       std::cout << "Instruction: " << llvmIRToString(node.first) << " in "
//            << node.first->getFunction()->getName().str() << "\n";
//       // Iterate call-string - flow fact set pairs
//       for (auto &flowfacts : node.second) {
//         std::cout << "Context: ";
//         // Print the elements of the call string
//         for (auto cstring : flowfacts.first.getInternalCS()) {
//           std::cout
//               << ((llvm::isa<llvm::Function>(cstring))
//                       ?
//                       llvm::dyn_cast<llvm::Function>(cstring)->getName().str()
//                       : IMProblem.CtoString(cstring))
//               << " * ";
//         }
//         std::cout << "\nFacts:\n";
//         // Print the elements of the corresponding set of flow facts
//         for (auto &flowfact : flowfacts.second) {
//           std::cout << IMProblem.DtoString(flowfact) << '\n';
//         }
//       }
//       std::cout << "\n\n";
//     }
//   }
// };

} // namespace psr

#endif
