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
#include <iostream>
#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/CallString.h>
#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>
#include <utility>
#include <vector>

namespace psr {

template <typename N, typename D, typename M, typename C, unsigned K,
          typename I>
class InterMonoSolver {
protected:
  InterMonoProblem<N, D, M, C, I> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  MonoMap<N, MonoMap<CallString<C, K>, MonoSet<D>>> Analysis;
  I ICFG;
  size_t prealloc_hint;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      std::vector<std::pair<N, N>> edges =
          ICFG.getAllControlFlowEdges(ICFG.getMethodOf(seed.first));
      Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      Analysis[seed.first][CallString<C, K>{ICFG.getMethodOf(seed.first)}]
          .insert(seed.second.begin(), seed.second.end());
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
  InterMonoSolver(InterMonoProblem<N, D, M, C, I> &IMP,
                  size_t prealloc_hint = 0)
      : IMProblem(IMP), ICFG(IMP.getICFG()), prealloc_hint(prealloc_hint) {}
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
      MonoMap<CallString<C, K>, MonoSet<D>> Out;
      // Add an id context to get the next loop to work
      Analysis[src][CallString<C, K>{ICFG.getMethodOf(src)}];
      for (auto context_entry : Analysis[src]) {
        auto context = context_entry.first;
        auto inter_context = context;
        if (ICFG.isCallStmt(src)) {
          // Handle call and call-to-ret flow
          if (!isIntraEdge(edge)) {
            inter_context.push(src);
            Out[inter_context] = IMProblem.callFlow(src, ICFG.getMethodOf(dst),
                                                    Analysis[src][context]);
          } else {
            Out[context] =
                IMProblem.callToRetFlow(src, dst, Analysis[src][context]);
          }
        } else if (ICFG.isExitStmt(src)) {
          // Handle return flow
          auto callsites = ICFG.getPredsOf(dst);
          auto callsite = (callsites.size() == 1) ? callsites[0] : nullptr;
          assert(callsite && "call-site not valid!");
          assert(ICFG.isCallStmt(callsite) == true && "call-site not found!");
          inter_context.pop();
          Out[inter_context] =
              IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst,
                                   Analysis[src][context]);
        } else {
          // Handle normal flow
          Out[context] = IMProblem.normalFlow(src, Analysis[src][context]);
        }
        // Check if data-flow facts have changed and if so add them to worklist
        // again. Caution: inter and intra edge must be distinguished, because
        // for inter edges the contexts changes.
        bool flowfactsstabilized =
            (isIntraEdge(edge))
                ? IMProblem.sqSubSetEqual(Out[context], Analysis[dst][context])
                : IMProblem.sqSubSetEqual(Out[inter_context],
                                          Analysis[dst][context]);
        if (!flowfactsstabilized) {
          // Only join facts which have the some context
          if (isIntraEdge(edge)) {
            Analysis[dst][context] =
                IMProblem.join(Analysis[dst][context], Out[context]);
          } else {
            if (isCallEdge(edge)) {
              Analysis[dst][inter_context] =
                  IMProblem.join(Analysis[dst][context], Out[inter_context]);
            } else {
              Analysis[dst][context] =
                  IMProblem.join(Analysis[dst][context], Out[inter_context]);
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
  }
};

} // namespace psr

#endif
