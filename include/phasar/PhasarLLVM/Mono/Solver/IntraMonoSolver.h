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
#include <iostream> // std::cout please remove it
#include <map>
#include <utility>
#include <vector>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>

namespace psr {

template <typename N, typename D, typename M, typename C>
class IntraMonoSolver {
protected:
  IntraMonoProblem<N, D, M, C> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  MonoMap<N, MonoSet<D>> Analysis;
  C CFG;
  size_t prealloc_hint;

  void initialize() {
    std::vector<std::pair<N, N>> edges =
        CFG.getAllControlFlowEdges(IMProblem.getFunction());
    // add all edges to the worklist
    Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
    // set all analysis information to the empty set
    for (auto s : CFG.getAllInstructionsOf(IMProblem.getFunction())) {
      Analysis.insert(std::make_pair(s, MonoSet<D>()));
    }
    if (prealloc_hint) {
      // for (auto &AnalysisSet : Analysis) {
      //   AnalysisSet.second.reserve(prealloc_hint);
      // }
    }
  }

public:
  IntraMonoSolver(IntraMonoProblem<N, D, M, C> &IMP, size_t prealloc_hint = 0)
      : IMProblem(IMP), CFG(IMP.getCFG()), prealloc_hint(prealloc_hint) {}
  virtual ~IntraMonoSolver() = default;
  virtual void solve() {
    // step 1: Initalization (of Worklist and Analysis)
    initialize();
    // step 2: Iteration (updating Worklist and Analysis)
    while (!Worklist.empty()) {
      std::cout << "worklist size: " << Worklist.size() << "\n";
      std::pair<N, N> path = Worklist.front();
      Worklist.pop_front();
      N src = path.first;
      N dst = path.second;
      MonoSet<D> Out = IMProblem.flow(src, Analysis[src]);
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
      entry.second = IMProblem.flow(entry.first, entry.second);
    }
  }
};

} // namespace psr

#endif
