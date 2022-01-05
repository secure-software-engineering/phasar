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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IFDSSOLVER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IFDSSOLVER_H

#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSToIDETabulationProblem.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

namespace psr {

template <typename OriginalAnalysisDomain> struct AnalysisDomainExtender;

template <typename AnalysisDomainTy>
class IFDSSolver : public IDESolver<AnalysisDomainExtender<AnalysisDomainTy>> {
public:
  using ProblemTy = IFDSTabulationProblem<AnalysisDomainTy>;
  using D = typename AnalysisDomainTy::d_t;
  using N = typename AnalysisDomainTy::n_t;

  IFDSSolver(IFDSTabulationProblem<AnalysisDomainTy> &IFDSProblem)
      : IDESolver<AnalysisDomainExtender<AnalysisDomainTy>>(IFDSProblem) {}

  ~IFDSSolver() override = default;

  /// Returns the data-flow results at the given statement.
  [[nodiscard]] virtual std::set<D> ifdsResultsAt(N Inst) {
    std::set<D> KeySet;
    std::unordered_map<D, BinaryDomain> ResultMap = this->resultsAt(Inst);
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
  template <typename NTy = N>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::remove_reference_t<NTy>, llvm::Instruction *>,
      std::set<D>>
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
    std::set<D> KeySet;
    for (auto &[FlowFact, LatticeValue] : getResultMap()) {
      KeySet.insert(FlowFact);
    }
    return KeySet;
  }
};

template <typename Problem>
IFDSSolver(Problem &) -> IFDSSolver<typename Problem::ProblemAnalysisDomain>;

template <typename Problem>
using IFDSSolver_P = IFDSSolver<typename Problem::ProblemAnalysisDomain>;

} // namespace psr

#endif
