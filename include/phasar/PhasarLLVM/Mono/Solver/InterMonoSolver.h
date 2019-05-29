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
#include <phasar/PhasarLLVM/Mono/Contexts/CallStringCTX.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <utility>
#include <vector>

namespace psr {

template <typename N, typename D, typename M, typename I>
class InterMonoSolver {
protected:
  InterMonoProblem<N, D, M, I> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  MonoMap<N, MonoMap<CallStringCTX<D, N, 3>, MonoSet<D>>> Analysis;
  MonoSet<M> AddedFunctions;
  I ICFG;
  CallStringCTX<D, N, 3> ZeroCTX;

  void initialize() {
    for (auto &seed : IMProblem.initialSeeds()) {
      std::vector<std::pair<N, N>> edges =
          ICFG.getAllControlFlowEdges(ICFG.getMethodOf(seed.first));
      Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      Analysis[seed.first][ZeroCTX].insert(seed.second.begin(), seed.second.end());
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

  void printWorkList() {
    std::cout << "CURRENT WORKLIST:" << std::endl;
    for (auto Entry : Worklist) {
      std::cout << llvmIRToString(Entry.first) << " ---> "
                << llvmIRToString(Entry.second) << std::endl;
    }
    std::cout << "-----------------" << std::endl;
  }

  void printMonoSet(const MonoSet<D> &S) {
    std::cout << "SET CONTENTS:\n{ ";
    for (auto Entry : S) {
      std::cout << llvmIRToString(Entry) << ", ";
    }
    std::cout << "}" << std::endl;
  }

public:
  InterMonoSolver(InterMonoProblem<N, D, M, I> &IMP)
      : IMProblem(IMP), ICFG(IMP.getICFG()) {}
  ~InterMonoSolver() = default;

  virtual void solve() {
    std::cout << "starting the InterMonoSolver::solve() procedure!\n";
    initialize();
    while (!Worklist.empty()) {
      std::pair<N, N> edge = Worklist.front();
      Worklist.pop_front();
      auto src = edge.first;
      auto dst = edge.second;
      if (ICFG.isCallStmt(src)) {
        // Add inter- and intra-edges of callee(s)
        for (auto callee : ICFG.getCalleesOfCallAt(src)) {
          if (AddedFunctions.count(callee)) {
            break;
          }
          AddedFunctions.insert(callee);
          // Add call edge(s)
          for (auto startPoint : ICFG.getStartPointsOf(callee)) {
            Worklist.push_back({src, startPoint});
          }
          // Add intra edges of callee
          std::vector<std::pair<N, N>> edges =
              ICFG.getAllControlFlowEdges(callee);
          Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
          // Add return edge(s)
          for (auto ret : ICFG.getExitPointsOf(callee)) {
            for (auto retSite : ICFG.getReturnSitesOfCallAt(src)) {
              Worklist.push_back({ret, retSite});
            }
          }
          // The call-to-return edge of the caller is already in the worklist
        }
      }
      // Compute the data-flow facts using the respective flow function
      MonoSet<D> Out;
      if (ICFG.isCallStmt(src)) {
        // Handle call and call-to-ret flow
        if (!isIntraEdge(edge)) {
          Out = IMProblem.callFlow(src, ICFG.getMethodOf(dst), Analysis[src][ZeroCTX]);
        } else {
          Out = IMProblem.callToRetFlow(src, dst, ICFG.getCalleesOfCallAt(src),
                                        Analysis[src][ZeroCTX]);
        }
      } else if (ICFG.isExitStmt(src)) {
        // Handle return flow
        auto callsites = ICFG.getPredsOf(dst);
        assert(callsites.size() && "call-site not valid, multiple call-sites!");
        auto callsite = callsites[0];
        assert(ICFG.isCallStmt(callsite) == true && "call-site not found!");
        Out = IMProblem.returnFlow(callsite, ICFG.getMethodOf(src), src, dst,
                                   Analysis[src][ZeroCTX]);
      } else {
        // Handle normal flow
        Out = IMProblem.normalFlow(src, Analysis[src][ZeroCTX]);
      }
      // Check if data-flow facts have changed and if so, add them to worklist
      // again.
      bool flowfactsstabilized = IMProblem.sqSubSetEqual(Out, Analysis[dst][ZeroCTX]);
      // std::cout << llvmIRToString(src) << " ---> " << llvmIRToString(dst)
      //           << " | stabilized: " << flowfactsstabilized << std::endl;
      // std::cout << "PRE: ";
      // printMonoSet(Analysis[dst]);
      // std::cout << "NEW: ";
      // printMonoSet(Out);
      // std::cout << std::endl;
      if (!flowfactsstabilized) {
        Analysis[dst][ZeroCTX] = IMProblem.join(Analysis[dst][ZeroCTX], Out);
        Worklist.push_back({src, dst});
        // add intra-procedural edges again
        for (auto nprimeprime : ICFG.getSuccsOf(dst)) {
          Worklist.push_back({dst, nprimeprime});
        }
        // add inter-procedural call edges again
        if (ICFG.isCallStmt(dst)) {
          for (auto callee : ICFG.getCalleesOfCallAt(dst)) {
            for (auto startPoint : ICFG.getStartPointsOf(callee)) {
              Worklist.push_back({dst, startPoint});
            }
          }
        }
        // add inter-procedural return edges again
        if (ICFG.isExitStmt(dst)) {
          for (auto caller : ICFG.getCallersOf(ICFG.getMethodOf(dst))) {
            for (auto nprimeprime : ICFG.getSuccsOf(caller)) {
              Worklist.push_back({dst, nprimeprime});
            }
          }
        }
      }
    }
  }
};

} // namespace psr

#endif
