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
#include "../CallStringPrefixedDFF.hh"
#include "../InterMonotoneProblem.hh"
#include <deque>
#include <iostream>
#include <utility>
#include <vector>
using namespace std;

template <typename N, typename D, typename M, typename I>
class InterMonotoneSolver {
protected:
  InterMonotoneProblem<N, D, M, I> &IMProblem;
  deque<pair<N, N>> Worklist;
  MonoMap<N, MonoSet<D>> Analysis;
  I ICFG;
  size_t prealloc_hint;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      vector<pair<N, N>> edges =
          ICFG.getAllControlFlowEdges(ICFG.getMethodOf(seed.first));
      Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      for (auto s : ICFG.getAllInstructionsOf(ICFG.getMethodOf(seed.first))) {
        Analysis.insert(std::make_pair(s, MonoSet<D>()));
      }
      Analysis[seed.first].insert(seed.second.begin(), seed.second.end());
    }
  }

  bool isIntraEdge(pair<N, N> edge) {
    return ICFG.getMethodOf(edge.first) == ICFG.getMethodOf(edge.second);
  }

public:
  InterMonotoneSolver(InterMonotoneProblem<N, D, M, I> &IMP,
                      size_t prealloc_hint = 0)
      : IMProblem(IMP), ICFG(IMP.getICFG()), prealloc_hint(prealloc_hint) {}
  ~InterMonotoneSolver() = default;

  virtual void solve() {
    cout << "starting the InterMonotoneSolver::solve() procedure!\n";
    initialize();
    while (!Worklist.empty()) {
      cout << "worklist size: " << Worklist.size() << "\n";
      pair<N, N> edge = Worklist.front();
      Worklist.pop_front();
      auto src = edge.first;
      auto dst = edge.second;

      MonoSet<D> Out;
      // handle call and call-to-ret flow
      if (ICFG.isCallStmt(src)) {
        if (!isIntraEdge(edge)) {
          Out = IMProblem.callFlow(src, ICFG.getMethodOf(dst), Analysis[src]);
        } else {
          Out = IMProblem.callToRetFlow(src, dst, Analysis[src]);
        }
      }
      // handle return flow
      if (ICFG.isExitStmt(src)) {
        auto callsites = ICFG.getPredsOf(dst);
        auto callsite = (callsites.size() == 1) ? callsites[0] : nullptr;
        assert(callsite && "call-site not valid!");
        assert(ICFG.isCallStmt(callsite) == CallType::call &&
               "call-site not found!");
        Out = IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst,
                                   Analysis[src]);
      }
      // handle normal flow
      if (!ICFG.isCallStmt(src) && !ICFG.isExitStmt(src)) {
        Out = IMProblem.normalFlow(src, Analysis[src]);
      }
      // check if data-flow facts have changed
      if (!IMProblem.sqSubSetEqual(Out, Analysis[dst])) {
        Analysis[dst] = IMProblem.join(Analysis[dst], Out);
        // handle function call
        if (ICFG.isCallStmt(dst)) {
          for (auto callee : ICFG.getCalleesOfCallAt(dst)) {
            // add call edges
            for (auto first_inst : ICFG.getStartPointsOf(callee)) {
              Worklist.push_back({dst, first_inst});
            }
            // add intra edges of callee
            vector<pair<N, N>> edges = ICFG.getAllControlFlowEdges(callee);
            Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
            // add return edges
            for (auto ret : ICFG.getExitPointsOf(callee)) {
              for (auto retsite : ICFG.getReturnSitesOfCallAt(dst)) {
                Worklist.push_back({ret, retsite});
              }
            }
          }
        }
        for (auto nprimeprime : ICFG.getSuccsOf(dst)) {
          Worklist.push_back({dst, nprimeprime});
        }
      }
    }
    // for (auto entry : Analysis) {
    //   entry.second = IMProblem.normalFlow(entry.first, entry.second);
    // }
  }
};

#endif
