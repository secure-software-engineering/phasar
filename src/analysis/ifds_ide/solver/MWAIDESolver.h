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
#include <set>
#include <type_traits>
#include <utility>
#include "../../../lib/LLVMShorthands.h"
#include "../../../utils/Logger.h"
#include "../../../utils/Table.h"
#include "../../misc/SummaryStrategy.h"
#include "../EdgeFunction.h"
#include "../EdgeFunctions.h"
#include "../FlowEdgeFunctionCache.h"
#include "../FlowFunctions.h"
#include "../IDETabulationProblem.h"
#include "../JoinLattice.h"
#include "../ZeroedFlowFunction.h"
#include "../edge_func/EdgeIdentity.h"
#include "../solver/JumpFunctions.h"
#include "IDESolver.h"
#include "JoinHandlingNode.h"
#include "LinkedNode.h"
#include "PathEdge.h"

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
          if (genStrategy == SummaryGenerationStrategy::always_all &&
              function != "main") {
            for (auto &arg : startPoint->getFunction()->args()) {
              initialValues.insert(&arg);
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
      startPoint->dump();
      for (const D &value : seed.second) {
        cout << "Value(s):" << endl;
        value->dump();
      }
    }
    IDESolver<N, D, M, V, I>::submitInitalSeeds();
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
