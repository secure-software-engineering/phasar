/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_IFDSSOLVER_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_IFDSSOLVER_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/Domain/BinaryDomain.h"

#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>

namespace psr {

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSSolver
    : public IDESolver<WithBinaryValueDomain<AnalysisDomainTy>, Container> {
public:
  using ProblemTy = IFDSTabulationProblem<AnalysisDomainTy>;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using i_t = typename AnalysisDomainTy::i_t;

  template <typename IfdsDomainTy,
            typename = std::enable_if_t<
                std::is_base_of_v<IfdsDomainTy, AnalysisDomainTy>>>
  IFDSSolver(IFDSTabulationProblem<IfdsDomainTy, Container> &IFDSProblem,
             const i_t *ICF)
      : IDESolver<WithBinaryValueDomain<AnalysisDomainTy>>(IFDSProblem, ICF) {}

  ~IFDSSolver() override = default;

  /// Returns the data-flow results at the given statement.
  [[nodiscard]] virtual std::set<d_t> ifdsResultsAt(n_t Inst) {
    std::set<d_t> KeySet;
    std::unordered_map<d_t, BinaryDomain> ResultMap = this->resultsAt(Inst);
    for (const auto &FlowFact : ResultMap) {
      KeySet.insert(FlowFact.first);
    }
    return KeySet;
  }

  /// Returns the data-flow results at the given statement while respecting
  /// LLVM's SSA semantics.
  ///
  /// An example: when a value is loaded and the location loaded from, here
  /// variable 'i', is a data-flow fact that holds, then the loaded value '%0'
  /// will usually be generated and also holds. However, due to the underlying
  /// theory (and respective implementation) this load instruction causes the
  /// loaded value to be generated and thus, it will be valid only AFTER the
  /// load instruction, i.e., at the successor instruction.
  ///
  ///   %0 = load i32, i32* %i, align 4
  ///
  /// This result accessor function returns the results at the successor
  /// instruction(s) reflecting that the expression on the left-hand side holds
  /// if the expression on the right-hand side holds.
  template <typename NTy = n_t>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::remove_reference_t<NTy>, llvm::Instruction *>,
      std::set<d_t>>
  ifdsResultsAtInLLVMSSA(NTy Inst) {
    auto getResultMap // NOLINT
        = [this, Inst]() {
            if (Inst->getType()->isVoidTy()) {
              return this->resultsAt(Inst);
            }
            // In this case we have a value on the left-hand side and must
            // return the results at the successor instruction. Note that
            // terminator instructions are always of void type.
            assert(Inst->getNextNode() &&
                   "Expected to find a valid successor node!");
            return this->resultsAt(Inst->getNextNode());
          };
    std::set<d_t> KeySet;
    for (auto &[FlowFact, LatticeValue] : getResultMap()) {
      KeySet.insert(FlowFact);
    }
    return KeySet;
  }
};

template <typename Problem, typename ICF>
IFDSSolver(Problem &, ICF *)
    -> IFDSSolver<typename Problem::ProblemAnalysisDomain,
                  typename Problem::container_type>;

template <typename Problem>
using IFDSSolver_P = IFDSSolver<typename Problem::ProblemAnalysisDomain,
                                typename Problem::container_type>;

template <typename AnalysisDomainTy, typename Container>
OwningSolverResults<typename AnalysisDomainTy::n_t,
                    typename AnalysisDomainTy::d_t,
                    typename AnalysisDomainTy::l_t>
solveIFDSProblem(IFDSTabulationProblem<AnalysisDomainTy, Container> &Problem,
                 const typename AnalysisDomainTy::i_t &ICF) {
  IFDSSolver<AnalysisDomainTy, Container> Solver(Problem, &ICF);
  Solver.solve();
  return Solver.consumeSolverResults();
}

} // namespace psr

#endif
