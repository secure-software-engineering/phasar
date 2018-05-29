/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef MWAIDESolver_H_
#define MWAIDESolver_H_

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

using namespace std;

template <typename N, typename D, typename M, typename V, typename I>
class MWAIDESolver : public IDESolver<N, D, M, V, I> {
private:
  enum SummaryGenerationStrategy genStrategy;

public:
  MWAIDESolver(IDETabulationProblem<N, D, M, V, I> &tabulationProblem,
               enum SummaryGenerationStrategy S)
      : IDESolver<N, D, M, V, I>(tabulationProblem), genStrategy(S) {
    cout << "MWAIDESolver::MWAIDESolver(IDETabulationProblem)" << endl;
  }

  virtual ~MWAIDESolver() = default;

  virtual void summarize() {
    cout << "MWAIDESolver::summarize()" << endl;
    // clear potential entry points and set the entry points within the code
    // under analysis
    IDESolver<N, D, M, V, I>::initialSeeds.clear();
    cout << "dependency ordered functions" << endl;
    for (auto function :
         IDESolver<N, D, M, V, I>::icfg.getDependencyOrderedFunctions()) {
      cout << function << endl;
      if (IDESolver<N, D, M, V, I>::icfg.getMethod(function)) {
        for (auto startPoint : IDESolver<N, D, M, V, I>::icfg.getStartPointsOf(
                 IDESolver<N, D, M, V, I>::icfg.getMethod(function))) {
          set<D> initialValues = {IDESolver<N, D, M, V, I>::zeroValue};
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
      cout << "Start point:" << endl;
      startPoint->print(llvm::outs());
      cout << "Value(s):" << endl;
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
    cout << "MWAIDESolver::submitInitalSeedsForSummary()" << endl;
    for (const auto &seed : this->initialSeeds) {
      N startPoint = seed.first;
      cout << "submitInitalSeedsForSummary - Start point:" << endl;
      startPoint->print(llvm::outs());
      for (const D &value : seed.second) {
        cout << "submitInitalSeedsForSummary - Value:" << endl;
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
    cout << "MWAIDESolver::combine()" << endl;
    cout << "Has precomputed summaries present: "
         << !IDESolver<N, D, M, V, I>::endsummarytab.empty() << endl;
    IDESolver<N, D, M, V, I>::submitInitalSeeds();
    if (IDESolver<N, D, M, V, I>::computevalues) {
      cout << "computing values" << endl;
      IDESolver<N, D, M, V, I>::computeValues();
    }
  }

  void setSummaries(Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> sum) {
    cout << "Set summaries!\n";
    IDESolver<N, D, M, V, I>::endsummarytab = sum;
  }

  Table<N, D, Table<N, D, shared_ptr<EdgeFunction<V>>>> getSummaries() {
    cout << "Get summaries!\n";
    return IDESolver<N, D, M, V, I>::endsummarytab;
  }

protected:
  MWAIDESolver(IFDSTabulationProblem<N, D, M, I> &tabulationProblem,
               enum SummaryGenerationStrategy S)
      : IDESolver<N, D, M, BinaryDomain, I>(tabulationProblem), genStrategy(S) {
    cout << "MWAIDESolver::MWAIDESolver(IFDSTabulationProblem)" << endl;
  }
};

#endif
