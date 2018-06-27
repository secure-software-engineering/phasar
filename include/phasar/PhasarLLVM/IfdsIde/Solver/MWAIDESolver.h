/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowEdgeFunctionCache.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/JoinHandlingNode.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/JumpFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LinkedNode.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/PathEdge.h>
#include <phasar/PhasarLLVM/IfdsIde/ZeroedFlowFunction.h>
#include <phasar/PhasarLLVM/Utils/SummaryStrategy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Table.h>
#include <set>
#include <type_traits>
#include <utility>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class MWAIDESolver : public IDESolver<N, D, M, V, I> {
private:
  enum SummaryGenerationStrategy genStrategy;

public:
  MWAIDESolver(IDETabulationProblem<N, D, M, V, I> &tabulationProblem,
               enum SummaryGenerationStrategy S)
      : IDESolver<N, D, M, V, I>(tabulationProblem), genStrategy(S) {
    std::cout << "MWAIDESolver::MWAIDESolver(IDETabulationProblem)" << std::endl;
  }

  virtual ~MWAIDESolver() = default;

  virtual void summarize() {
    std::cout << "MWAIDESolver::summarize()" << std::endl;
    // clear potential entry points and std::set the entry points within the code
    // under analysis
    IDESolver<N, D, M, V, I>::initialSeeds.clear();
    std::cout << "dependency ordered functions" << std::endl;
    for (auto function :
         IDESolver<N, D, M, V, I>::icfg.getDependencyOrderedFunctions()) {
      std::cout << function << std::endl;
      if (IDESolver<N, D, M, V, I>::icfg.getMethod(function)) {
        for (auto startPoint : IDESolver<N, D, M, V, I>::icfg.getStartPointsOf(
                 IDESolver<N, D, M, V, I>::icfg.getMethod(function))) {
          std::set<D> initialValues = {IDESolver<N, D, M, V, I>::zeroValue};
          if (function != "main") {
            switch (genStrategy) {
            case SummaryGenerationStrategy::always_all:
              for (auto &arg : startPoint->getFunction()->args()) {
                initialValues.insert(&arg);
              }
              break;
            case SummaryGenerationStrategy::always_none:
              // nothing to do, just the zeroValue
              break;
            case SummaryGenerationStrategy::all_and_none:
              break;
            case SummaryGenerationStrategy::powerset:
              break;
            case SummaryGenerationStrategy::all_observed:
              break;
            default:
              break;
            }
          }
          IDESolver<N, D, M, V, I>::initialSeeds.insert(
              std::make_pair(startPoint, initialValues));
        }
      }
    }
    // We start our analysis and construct exploded supergraph
    for (const auto &seed : IDESolver<N, D, M, V, I>::initialSeeds) {
      N startPoint = seed.first;
      std::cout << "Start point:" << std::endl;
      startPoint->print(llvm::outs());
      std::cout << "Value(s):" << std::endl;
      for (const D &value : seed.second) {
        value->print(llvm::outs());
      }
    }
    this->submitInitalSeedsForSummary();
  }

  /**
   * Schedules the processing of initial seeds, initiating the analysis.
   * Clients should only call this methods if performing synchronization on
   * their own. Normally, solve() should be called instead.
   */
  void submitInitalSeedsForSummary() {
    std::cout << "MWAIDESolver::submitInitalSeedsForSummary()" << std::endl;
    for (const auto &seed : this->initialSeeds) {
      N startPoint = seed.first;
      std::cout << "submitInitalSeedsForSummary - Start point:" << std::endl;
      startPoint->print(llvm::outs());
      for (const D &value : seed.second) {
        std::cout << "submitInitalSeedsForSummary - Value:" << std::endl;
        value->print(llvm::outs());
        this->propagate(value, startPoint, value,
                        EdgeIdentity<V>::getInstance(), nullptr, false);
      }
      this->jumpFn->addFunction(this->zeroValue, startPoint, this->zeroValue,
                                EdgeIdentity<V>::getInstance());
    }
  }

  /**
   * @brief Combines MWA information and performs a final repropagation step.
   */
  virtual void combine() {
    std::cout << "MWAIDESolver::combine()" << std::endl;
    std::cout << "Has precomputed summaries present: "
         << !IDESolver<N, D, M, V, I>::endsummarytab.empty() << std::endl;
    IDESolver<N, D, M, V, I>::submitInitalSeeds();
    if (IDESolver<N, D, M, V, I>::computevalues) {
      std::cout << "computing values" << std::endl;
      IDESolver<N, D, M, V, I>::computeValues();
    }
  }

  void setSummaries(Table<N, D, Table<N, D, std::shared_ptr<EdgeFunction<V>>>> sum) {
    std::cout << "Set summaries!\n";
    IDESolver<N, D, M, V, I>::endsummarytab = sum;
  }

  Table<N, D, Table<N, D, std::shared_ptr<EdgeFunction<V>>>> getSummaries() {
    std::cout << "Get summaries!\n";
    return IDESolver<N, D, M, V, I>::endsummarytab;
  }

protected:
  MWAIDESolver(IFDSTabulationProblem<N, D, M, I> &tabulationProblem,
               enum SummaryGenerationStrategy S)
      : IDESolver<N, D, M, BinaryDomain, I>(tabulationProblem), genStrategy(S) {
    std::cout << "MWAIDESolver::MWAIDESolver(IFDSTabulationProblem)" << std::endl;
  }
};

} // namespace psr
