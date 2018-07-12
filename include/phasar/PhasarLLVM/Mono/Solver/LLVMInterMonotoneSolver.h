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

#include <iostream>
#include <memory>

#include <llvm/IR/Instruction.h>

#include "../InterMonotoneProblem.h"
#include "InterMonotoneGeneralizedSolver.h"
#include "../Contexts/CallString.h"
#include "../Contexts/ValueBasedContext.h"
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>

namespace psr {

struct LLVMOrderingById {
  using edge_t = std::pair<const llvm::Instruction* , const llvm::Instruction*>;
  bool lessInstruction(const llvm::Instruction* lhs, const llvm::Instruction* rhs) const {
    return std::stol(getMetaDataID(lhs)) < std::stol(getMetaDataID(rhs));
  }

  bool operator() (const edge_t& lhs, const edge_t& rhs) const {
    return lessInstruction(lhs.first, rhs.first) || (!lessInstruction(rhs.first, lhs.first) && lessInstruction(lhs.second, rhs.second));
  }
};

template <typename V, class Context, class Ordering = LLVMOrderingById>
class LLVMInterMonotoneSolver
  : public InterMonotoneGeneralizedSolver<
                                    InterMonotoneProblem<const llvm::Instruction *, V,
                                    const llvm::Function *, LLVMBasedICFG>,
                                    Context, Ordering> {
  public:
    using IMSBase_t = InterMonotoneGeneralizedSolver<
                                      InterMonotoneProblem<const llvm::Instruction *, V,
                                      const llvm::Function *, LLVMBasedICFG>,
                                      Context, Ordering>;

  protected:
    bool DUMP_RESULTS;
  public:
    LLVMInterMonotoneSolver(typename IMSBase_t::IMP_t &IMP,
                            Context &_Context,
                            const llvm::Function *Method,
                            bool dump = false)
        : IMSBase_t(IMP, _Context, Method),
          DUMP_RESULTS(dump) {}
    virtual ~LLVMInterMonotoneSolver() = default;

    LLVMInterMonotoneSolver(const LLVMInterMonotoneSolver& copy) = delete;
    LLVMInterMonotoneSolver(LLVMInterMonotoneSolver& move) = delete;
    LLVMInterMonotoneSolver& operator=(const LLVMInterMonotoneSolver& copy) = delete;
    LLVMInterMonotoneSolver& operator=(LLVMInterMonotoneSolver&& move) = delete;

    void dump() {};
};

// template <typename V, unsigned K, class Ordering = LLVMOrderingById>
// class LLVMInterMonotoneCallStringSolver
//   : public LLVMInterMonotoneSolver<V, CallString<const llvm::Instruction *, V, K>> {
//   public:
//     LLVMInterMonotoneCallStringSolver();
//     virtual ~LLVMInterMonotoneCallStringSolver() = default;
//
//     LLVMInterMonotoneCallStringSolver(const LLVMInterMonotoneCallStringSolver& copy) = delete;
//     LLVMInterMonotoneCallStringSolver(LLVMInterMonotoneCallStringSolver& move) = delete;
//     LLVMInterMonotoneCallStringSolver& operator=(const LLVMInterMonotoneCallStringSolver& copy) = delete;
//     LLVMInterMonotoneCallStringSolver& operator=(LLVMInterMonotoneCallStringSolver&& move) = delete;
// };
//
// template <typename V, class Ordering = LLVMOrderingById>
// class LLVMInterMonotoneValueBasedContextSolver
//   : public LLVMInterMonotoneSolver<V, ValueBasedContext<const llvm::Instruction *, V>> {
//   public:
//     LLVMInterMonotoneValueBasedContextSolver() = default;
//     virtual ~LLVMInterMonotoneValueBasedContextSolver() = default;
//
//     LLVMInterMonotoneValueBasedContextSolver(const LLVMInterMonotoneValueBasedContextSolver& copy) = delete;
//     LLVMInterMonotoneValueBasedContextSolver(LLVMInterMonotoneValueBasedContextSolver& move) = delete;
//     LLVMInterMonotoneValueBasedContextSolver& operator=(const LLVMInterMonotoneValueBasedContextSolver& copy) = delete;
//     LLVMInterMonotoneValueBasedContextSolver& operator=(LLVMInterMonotoneValueBasedContextSolver&& move) = delete;
// };
//
// template <typename V, class Ordering = LLVMOrderingById>
// class LLVMInterMonotoneMappingValueBasedContextSolver
//   : public LLVMInterMonotoneSolver<V, MappingValueBasedContext<const llvm::Instruction *, V>> {
//   private:
//     template <typename T1, typename T2>
//     void InterMonotoneGeneralizedSolver_check() {
//       static_assert(std::is_base_of<ValueBase<T1, T2, V>, V>::value, "Template class V must be a sub class of ValueBase<T1, T2, V> with T1, T2 templates\n");
//     }
//
//   public:
//     LLVMInterMonotoneMappingValueBasedContextSolver() = default;
//     virtual ~LLVMInterMonotoneMappingValueBasedContextSolver() = default;
//
//     LLVMInterMonotoneMappingValueBasedContextSolver(const LLVMInterMonotoneMappingValueBasedContextSolver& copy) = delete;
//     LLVMInterMonotoneMappingValueBasedContextSolver(LLVMInterMonotoneMappingValueBasedContextSolver& move) = delete;
//     LLVMInterMonotoneMappingValueBasedContextSolver& operator=(const LLVMInterMonotoneMappingValueBasedContextSolver& copy) = delete;
//     LLVMInterMonotoneMappingValueBasedContextSolver& operator=(LLVMInterMonotoneMappingValueBasedContextSolver&& move) = delete;
// };

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
//         for (auto cstring : flowfacts.first.getInternalCS()) {
//           std::cout
//               << ((llvm::isa<llvm::Function>(cstring))
//                       ? llvm::dyn_cast<llvm::Function>(cstring)->getName().str()
//                       : IMProblem.CtoString(cstring))
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

template<typename IMP_t, typename Context_t, typename Ordering_t = LLVMOrderingById>
auto make_LLVMBasedIMS(IMP_t &IMP, Context_t &Context, typename IMP_t::Method_t Method, bool dump = false)
-> std::unique_ptr<LLVMInterMonotoneSolver<typename IMP_t::Value_t, Context_t, Ordering_t>> {
  using ptr_t = LLVMInterMonotoneSolver<typename IMP_t::Value_t, Context_t, Ordering_t>;
  return std::make_unique<ptr_t>(IMP, Context, Method, dump);
}

} // namespace psr
