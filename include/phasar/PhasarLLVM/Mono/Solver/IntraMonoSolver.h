/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoSolver.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_INTRAMONOSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_INTRAMONOSOLVER_H_

#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>

#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>
#include <phasar/Utils/BitVectorSet.h>

namespace psr {

template <typename N, typename D, typename M, typename C>
class IntraMonoSolver {
protected:
  IntraMonoProblem<N, D, M, C> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  std::unordered_map<N, BitVectorSet<D>> Analysis;
  C CFG;

  void initialize() {
    std::vector<std::pair<N, N>> edges =
        CFG.getAllControlFlowEdges(IMProblem.getFunction());
    // add all edges to the worklist
    Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
    // set all analysis information to the empty set
    for (auto s : CFG.getAllInstructionsOf(IMProblem.getFunction())) {
      Analysis.insert(std::make_pair(s, BitVectorSet<D>()));
    }
    // insert initial seeds
    for (auto &seed : IMProblem.initialSeeds()) {
      Analysis[seed.first].insert(seed.second);
    }
  }

public:
  IntraMonoSolver(IntraMonoProblem<N, D, M, C> &IMP)
      : IMProblem(IMP), CFG(IMP.getCFG()) {}
  virtual ~IntraMonoSolver() = default;
  virtual void solve() {
    // step 1: Initalization (of Worklist and Analysis)
    initialize();
    // step 2: Iteration (updating Worklist and Analysis)
    while (!Worklist.empty()) {
      // std::cout << "worklist size: " << Worklist.size() << "\n";
      std::pair<N, N> path = Worklist.front();
      Worklist.pop_front();
      N src = path.first;
      N dst = path.second;
      BitVectorSet<D> Out = IMProblem.normalFlow(src, Analysis[src]);
      if (!IMProblem.sqSubSetEqual(Out, Analysis[dst])) {
        Analysis[dst] = IMProblem.join(Analysis[dst], Out);
        for (auto nprimeprime : CFG.getSuccsOf(dst)) {
          Worklist.push_back({dst, nprimeprime});
        }
      }
    }
    // step 3: Presenting the result (MFP_in and MFP_out)
    // MFP_in[s] = Analysis[s];
    // MFP out[s] = IMProblem.flow(Analysis[s]);
    for (auto entry : Analysis) {
      entry.second = IMProblem.normalFlow(entry.first, entry.second);
    }
  }

  BitVectorSet<D> getResultsAt(N n) { return Analysis[n]; }
};

} // namespace psr

#endif
