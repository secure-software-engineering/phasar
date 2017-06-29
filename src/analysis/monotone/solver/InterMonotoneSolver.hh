/*
 * InterMonotoneSolver.hh
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef INTERMONOTONESOLVER_HH_
#define INTERMONOTONESOLVER_HH_

#include "../../../utils/ContainerConfiguration.hh"
#include "../InterMonotoneProblem.hh"
#include "../CallString.hh"
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
  deque<pair<N, ctx_type>> Worklist;
  MonoMap<N, MonoMap<ctx_type, MonoSet<D>>> Analysis;
  I ICFG;
  size_t prealloc_hint;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      Worklist.push_back(make_pair(seed.first, ICFG.getMethodOf(seed.first)));
      Analysis[seed.first].insert(seed.second);
    }
  }

public:
  InterMonotoneSolver(InterMonotoneProblem<N, D, M, I> &IMP,
                      size_t prealloc_hint = 0)
      : IMProblem(IMP), ICFG(IMP.getICFG()), prealloc_hint(prealloc_hint) {}
  ~InterMonotoneSolver() = default;
  
  virtual void solve() {
    cout << "starting the InterMonotoneSolver::solve() procedure!\n";
    while(!Worklist.empty()) {
      cout << "worklist size: " << Worklist.size() << "\n";
      pair<N, ctx_type> src = Worklist.front();
      N src_node = src.first;
      ctx_type ctx = src.second;
      Worklist.pop_front();
      // if (ICFG.isCallStmt(src_node)) {
      //   for (M callee : ICFG.getCalleesOfCallAt(src_node)) {
      //     MonoSet<D> Out = IMProblem.callFlow(src_node, calle, Analysis[src_node][ctx]);

      //   }
      // } else if (ICFG.isExitStmt(src_node)) {
      //   // MonoSet<D> Out = IMProblem.returnFlow()
      // } else {
      MonoSet<D> Out = IMProblem.normalFlow(src_node, Analysis[src_node][ctx]);
      if (!IMProblem.sqSubSetEqual(Analysis[src_node][ctx], Out)) {
        Analysis[src_node][ctx] = Out;
        Worklist.push_back(src);
      }
      // }
    }
  }
};

#endif
