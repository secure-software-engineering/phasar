/*
 * MonotoneSolver.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef INTRAMONOTONESOLVER_HH_
#define INTRAMONOTONESOLVER_HH_

#include "../../../utils/ContainerConfiguration.hh"
#include "../IntraMonotoneProblem.hh"
#include <deque>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
using namespace std;

template <typename N, typename D, typename M, typename C>
class IntraMonotoneSolver {
protected:
  IntraMonotoneProblem<N, D, M, C> &IMProblem;
  deque<pair<N, N>> Worklist;
  MonoMap<N, MonoSet<D>> Analysis;
  C CFG;
  size_t prealloc_hint;

  void initialize() {
    vector<pair<N, N>> edges =
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
  IntraMonotoneSolver(IntraMonotoneProblem<N, D, M, C> &IMP,
                      size_t prealloc_hint = 0)
      : IMProblem(IMP), CFG(IMP.getCFG()), prealloc_hint(prealloc_hint) {}
  virtual ~IntraMonotoneSolver() = default;
  virtual void solve() {
    // step 1: Initalization (of Worklist and Analysis)
    initialize();
    // step 2: Iteration (updating Worklist and Analysis)
    while (!Worklist.empty()) {
      cout << "worklist size: " << Worklist.size() << "\n";
      pair<N, N> path = Worklist.front();
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

#endif
