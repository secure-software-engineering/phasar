/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoSolver.h
 *
 *  Created on: 19.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_INTERMONOSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_INTERMONOSOLVER_H_

#include <deque>
#include <iosfwd>
#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>
#include <utility>
#include <vector>

namespace psr {

template <typename N, typename D, typename M, typename I>
class InterMonoSolver {
protected:
  InterMonoProblem<N, D, M, I> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  MonoMap<N, MonoSet<D>> Analysis;
  I ICFG;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      std::vector<std::pair<N, N>> edges =
          ICFG.getAllControlFlowEdges(ICFG.getMethodOf(seed.first));
      Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      Analysis[seed.first].insert(seed.second.begin(), seed.second.end());
    }
  }

  bool isIntraEdge(std::pair<N, N> edge) {
    return ICFG.getMethodOf(edge.first) == ICFG.getMethodOf(edge.second);
  }

  bool isCallEdge(std::pair<N, N> edge) {
    return !isIntraEdge(edge) && ICFG.isCallStmt(edge.first);
  }

  bool isReturnEdge(std::pair<N, N> edge) {
    return !isIntraEdge(edge) && ICFG.isExitStmt(edge.first);
  }

public:
  InterMonoSolver(InterMonoProblem<N, D, M, I> &IMP)
      : IMProblem(IMP), ICFG(IMP.getICFG()) {}
  ~InterMonoSolver() = default;

  virtual void solve() {
    std::cout << "starting the InterMonoSolver::solve() procedure!\n";
    initialize();
    while (!Worklist.empty()) {
      std::cout << "worklist size: " << Worklist.size() << "\n";
      std::pair<N, N> edge = Worklist.front();
      Worklist.pop_front();
      auto src = edge.first;
      auto dst = edge.second;
      std::cout << "process edge (intra=" << isIntraEdge(edge) << ") <"
                << llvmIRToString(src) << "> ---> <" << llvmIRToString(dst)
                << ">\n";

      MonoSet<D> Out;

      if (ICFG.isCallStmt(src)) {
        // Handle call and call-to-ret flow
        if (!isIntraEdge(edge)) {
          Out = IMProblem.callFlow(src, ICFG.getMethodOf(dst), Analysis[src]);
        } else {
          Out = IMProblem.callToRetFlow(src, dst, Analysis[src]);
        }
      } else if (ICFG.isExitStmt(src)) {
        // Handle return flow
        auto callsites = ICFG.getPredsOf(dst);
        auto callsite = (callsites.size() == 1) ? callsites[0] : nullptr;
        assert(callsite && "call-site not valid!");
        assert(ICFG.isCallStmt(callsite) == true && "call-site not found!");
        Out = IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst, Analysis[src]);
      } else {
        // Handle normal flow
        Out = IMProblem.normalFlow(src, Analysis[src]);
      }
      // Check if data-flow facts have changed and if so add them to worklist
      // again.
      bool flowfactsstabilized = IMProblem.sqSubSetEqual(Out, Analysis[dst]);

      if (!flowfactsstabilized) {
        if (isIntraEdge(edge)) {
          Analysis[dst]=
              IMProblem.join(Analysis[dst], Out);
        } else {
          if (isCallEdge(edge)) {
            Analysis[dst] =
                IMProblem.join(Analysis[dst], Out);
          } else {
            Analysis[dst] =
                IMProblem.join(Analysis[dst], Out);
          }
        }
        // Handle function call and add inter call edges
        if (ICFG.isCallStmt(dst)) {
          for (auto callee : ICFG.getCalleesOfCallAt(dst)) {
            // Add call edges
            for (auto first_inst : ICFG.getStartPointsOf(callee)) {
              Worklist.push_back({dst, first_inst});
            }
            // Add intra edges of callee
            std::vector<std::pair<N, N>> edges =
                ICFG.getAllControlFlowEdges(callee);
            Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
            // Add inter return edges
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
  }
};

} // namespace psr

#endif
