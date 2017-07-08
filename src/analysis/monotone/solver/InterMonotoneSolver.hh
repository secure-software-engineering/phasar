/*
 * InterMonotoneSolver.hh
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef INTERMONOTONESOLVER_HH_
#define INTERMONOTONESOLVER_HH_

#include "../../../utils/ContainerConfiguration.hh"
#include "../CallString.hh"
#include "../InterMonotoneProblem.hh"
#include <deque>
#include <iostream>
#include <utility>
#include <vector>
using namespace std;

template <typename N, typename D, typename M, typename I>
class InterMonotoneSolver {
private:
  typedef CallString<M, 4> ctx_type;

protected:
  InterMonotoneProblem<N, D, M, I> &IMProblem;
  deque<N> Worklist;
  MonoMap<N, MonoSet<D>> Analysis;
  I ICFG;
  size_t prealloc_hint;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      Worklist.push_back(seed.first);
      Analysis[seed.first].insert(seed.second.begin(), seed.second.end());
    }
  }

public:
  InterMonotoneSolver(InterMonotoneProblem<N, D, M, I> &IMP,
                      size_t prealloc_hint = 0)
      : IMProblem(IMP), ICFG(IMP.getICFG()), prealloc_hint(prealloc_hint) {}
  ~InterMonotoneSolver() = default;

  virtual void solve() {
    cout << "starting the InterMonotoneSolver::solve() procedure!\n";
    initialize();
    // while (!Worklist.empty()) {
    //   cout << "worklist size: " << Worklist.size() << "\n";
    //   N src_node = Worklist.front();
    //   Worklist.pop_front();
    //   // if (ICFG.isCallStmt(src_node) == CallType::call) {
    //   //   for (M callee : ICFG.getCalleesOfCallAt(src_node)) {
    //   //     // MonoSet<D> Out = IMProblem.callFlow(src_node, callee,
    //   //     // Analysis[src_node]);
    //   //   }
    //   //   // recursive call
    //   //   // the call
    //   //   // MonoSet<D> Out = IMProblem.callToRetFlow();
    //   // } else if (ICFG.isExitStmt(src_node)) {
    //   //   // MonoSet<D> Out = IMProblem.returnFlow()
    //   // } else {
    //   auto target_nodes = ICFG.getSuccsOf(src_node);
    //   MonoSet<D> Out = IMProblem.normalFlow(src_node, Analysis[src_node]);
    //   for (auto target_node : target_nodes) {
    //     if (!IMProblem.sqSubSetEqual(Out, Analysis[target_node])) {
    //       Analysis[target_node] = Out;
    //       Worklist.push_back(target_node);
    //     }
    //   }
    // }
  }
};

#endif
