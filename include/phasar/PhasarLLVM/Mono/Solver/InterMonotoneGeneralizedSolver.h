/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * InterMonotoneSolver.h
 *
 *  Created on: 15.06.2018
 *      Author: nicolas
 */

#ifndef INTERMONOTONEGENERALIZEDSOLVER_H_
#define INTERMONOTONEGENERALIZEDSOLVER_H_

#include <deque>
#include <iostream>
#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>
#include <utility>
#include <vector>
using namespace std;

namespace psr {

template <typename N, typename D, typename M, typename C, typename I,
          typename Context>
class InterMonotoneGeneralizedSolver {
protected:
  InterMonotoneProblem<N, D, M, C, I> &IMProblem;
  deque<pair<N, N>> Worklist;
  // set<pair<N, N>> Worklist;
  MonoMap<N, MonoMap<Context, MonoSet<D>>> Analysis;
  I ICFG;
  set<M> visited_once;
  // size_t prealloc_hint;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      M method = ICFG.getMethodOf(seed.first);
      if ( !visited_once.count(method) ) {
        visited_once.insert(ICFG.getMethodOf(seed.first));
        vector<pair<N, N>> edges =
            ICFG.getAllControlFlowEdges(method);
        Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      }
      Analysis[seed.first][Context{method}]
          .insert(seed.second.begin(), seed.second.end());
      cout << " *** One Seed deployed *** \n";
    }
  }

  bool isIntraEdge(pair<N, N> edge) {
    return ICFG.getMethodOf(edge.first) == ICFG.getMethodOf(edge.second);
  }

  bool isCallEdge(pair<N, N> edge) {
    return !isIntraEdge(edge) && ICFG.isCallStmt(edge.first);
  }

  bool isReturnEdge(pair<N, N> edge) {
    return !isIntraEdge(edge) && ICFG.isExitStmt(edge.first);
  }

public:
  InterMonotoneGeneralizedSolver(InterMonotoneProblem<N, D, M, C, I> &IMP)
                      // size_t prealloc_hint = 0)
      : IMProblem(IMP), ICFG(IMP.getICFG()) {} //, prealloc_hint(prealloc_hint) {}
  ~InterMonotoneGeneralizedSolver() = default;

  // DEBUG : Only for test purpose, should be a special fonction
  // To think later, probably require to provide an implementation of
  // this fonction
  // int static compareN(const N lhs, const N rhs) {
  //   if (llvm::getMetaDataId(lhs) < llvm::getMetaDataId(rhs))
  //     return -1;
  //   if (llvm::getMetaDataId(lhs) > llvm::getMetaDataId(rhs))
  //     return 1;
  //   return 0;
  // }
  // // DEBUG
  //
  // bool static lessPairN(const pair<N, N> lhs, const pair<N, N> rhs) {
  //   const int comp_first = 2*compareN(lhs.first, rhs.first);
  //   const int comp_second = compareN(lhs.second, rhs.second);
  //   const int final_comp = comp_first + comp_second;
  //   return final_comp < 0;
  // }

  virtual void solve() {
    cout << "starting the InterMonotoneSolver::solve() procedure!\n";
    initialize();
    while (!Worklist.empty()) {
      // DEBUG
      cout << "worklist size: " << Worklist.size() << "\n";
      // DEBUG
      pair<N, N> edge = Worklist.front();
      Worklist.pop_front();
      auto src = edge.first;
      auto dst = edge.second;
      // DEBUG
      cout << "process edge (intra=" << isIntraEdge(edge) << ") <"
           << llvmIRToString(src) << "> ---> <" << llvmIRToString(dst) << ">\n";
      // DEBUG
      MonoMap<Context, MonoSet<D>> Out;
      // Add an id context to get the next loop to work
      Analysis[src][Context{ICFG.getMethodOf(src)}];
      for (auto context_entry : Analysis[src]) {
        auto context = context_entry.first;
        auto inter_context = context;
        if (ICFG.isCallStmt(src)) {
          // Handle call and call-to-ret flow
          if (!isIntraEdge(edge)) {
            inter_context.enterFunction(src, dst, Analysis[src][context]);
            /* WARNING : this is the interface for the std::map, boost::map or
             *   other library may not work with count to check the existence of
             *   the key in the map
             */
            if ( Analysis[src].count(inter_context) ) {
              Out[inter_context] = IMProblem.callFlow(src, ICFG.getMethodOf(dst),
                                                      Analysis[src][context],
                                                      Analysis[src][inter_context]);
            } else {
              Out[inter_context] = IMProblem.callFlow(src, ICFG.getMethodOf(dst),
                                                      Analysis[src][context]);
            }
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
          inter_context.exitFunction(src, dst, Analysis[src][context]);

          if ( Analysis[src].count(inter_context) ) {
            Out[inter_context] =
                IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst,
                                     Analysis[src][context],
                                     Analysis[src][inter_context]);
          } else {
            Out[inter_context] =
                IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst,
                                     Analysis[src][context]);
          }

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
                                          Analysis[dst][inter_context]);
        if (!flowfactsstabilized) {
          // Only join facts which have the same context
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
              vector<pair<N, N>> edges = ICFG.getAllControlFlowEdges(callee);
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

        if ( ICFG.isCallStmt(dst) ) {
          for (auto callee : ICFG.getCalleesOfCallAt(dst)) {
            if ( !visited_once.count(callee) ) {
              visited_once.insert(callee);
              for (auto first_inst : ICFG.getStartPointsOf(callee)) {
                Worklist.push_back({dst, first_inst});
              }

              vector<pair<N, N>> edges = ICFG.getAllControlFlowEdges(callee);
              Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
              // Add inter return edges
              for (auto ret : ICFG.getExitPointsOf(callee)) {
                for (auto retsite : ICFG.getReturnSitesOfCallAt(dst)) {
                  Worklist.push_back({ret, retsite});
                }
              }
            }
          }
        }
      }
    }
  }
};

} // namespace psr

#endif
