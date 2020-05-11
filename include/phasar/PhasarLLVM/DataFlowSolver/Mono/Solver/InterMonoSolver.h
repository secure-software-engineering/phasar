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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Contexts/CallStringCTX.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename I, typename ContainerTy, unsigned K>
class InterMonoSolver {
public:
  using ProblemTy = InterMonoProblem<N, D, F, T, V, I, ContainerTy>;

protected:
  InterMonoProblem<N, D, F, T, V, I, ContainerTy> &IMProblem;
  std::deque<std::pair<N, N>> Worklist;
  std::unordered_map<N, std::unordered_map<CallStringCTX<N, K>, ContainerTy>>
      Analysis;
  std::unordered_set<F> AddedFunctions;
  const I *ICF;

  void initialize() {
    for (auto &[Node, FlowFacts] : IMProblem.initialSeeds()) {
      auto ControlFlowEdges =
          ICF->getAllControlFlowEdges(ICF->getFunctionOf(Node));
      Worklist.insert(Worklist.begin(), ControlFlowEdges.begin(),
                      ControlFlowEdges.end());
      // Initialize with empty context and empty data-flow set such that the
      // flow functions are at least called once per instruction
      for (auto &[Src, Dst] : ControlFlowEdges) {
        Analysis[Src][CallStringCTX<N, K>()];
      }
      // Initialize last
      if (!ControlFlowEdges.empty()) {
        Analysis[ControlFlowEdges.back().second][CallStringCTX<N, K>()];
      }
      // Additionally, insert the initial seeds
      Analysis[Node][CallStringCTX<N, K>()].insert(FlowFacts.begin(),
                                                   FlowFacts.end());
    }
  }

  bool isIntraEdge(std::pair<N, N> Edge) {
    return ICF->getFunctionOf(Edge.first) == ICF->getFunctionOf(Edge.second);
  }

  bool isCallEdge(std::pair<N, N> Edge) {
    return !isIntraEdge(Edge) && ICF->isCallStmt(Edge.first);
  }

  bool isReturnEdge(std::pair<N, N> Edge) {
    return !isIntraEdge(Edge) && ICF->isExitStmt(Edge.first);
  }

  void printWorkList() {
    std::cout << "CURRENT WORKLIST:\n";
    for (auto &[Src, Dst] : Worklist) {
      std::cout << llvmIRToString(Src) << " --> " << llvmIRToString(Dst)
                << '\n';
    }
    std::cout << "-----------------\n";
  }

  void addCalleesToWorklist(std::pair<N, N> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    // Add inter- and intra-edges of callee(s)
    for (auto Callee : ICF->getCalleesOfCallAt(Src)) {
      if (AddedFunctions.count(Callee)) {
        break;
      }
      AddedFunctions.insert(Callee);
      // Add call Edge(s)
      for (auto StartPoint : ICF->getStartPointsOf(Callee)) {
        Worklist.push_back({Src, StartPoint});
      }
      // Add intra edges of callee
      std::vector<std::pair<N, N>> edges = ICF->getAllControlFlowEdges(Callee);
      Worklist.insert(Worklist.begin(), edges.begin(), edges.end());
      // Initialize with empty context and empty data-flow set such that the
      // flow functions are at least called once per instruction
      for (auto &Edge : edges) {
        Analysis[Edge.first][CallStringCTX<N, K>()];
      }
      // Initialize last
      if (!edges.empty()) {
        Analysis[edges.back().second][CallStringCTX<N, K>()];
      }
      // Add return Edge(s)
      for (auto ret : ICF->getExitPointsOf(Callee)) {
        for (auto retSite : ICF->getReturnSitesOfCallAt(Src)) {
          Worklist.push_back({ret, retSite});
        }
      }
    }
    // The (intra-procedural) call-to-return Edge of the caller is already in
    // the worklist
  }

  void addToWorklist(std::pair<N, N> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    Worklist.push_back({Src, Dst});
    // add intra-procedural edges again
    for (auto Nprimeprime : ICF->getSuccsOf(Dst)) {
      Worklist.push_back({Dst, Nprimeprime});
    }
    // add inter-procedural call edges again
    if (ICF->isCallStmt(Dst)) {
      for (auto Callee : ICF->getCalleesOfCallAt(Dst)) {
        for (auto StartPoint : ICF->getStartPointsOf(Callee)) {
          Worklist.push_back({Dst, StartPoint});
        }
      }
    }
    // add inter-procedural return edges again
    if (ICF->isExitStmt(Dst)) {
      for (auto caller : ICF->getCallersOf(ICF->getFunctionOf(Dst))) {
        for (auto Nprimeprime : ICF->getSuccsOf(caller)) {
          Worklist.push_back({Dst, Nprimeprime});
        }
      }
    }
  }

public:
  InterMonoSolver(InterMonoProblem<N, D, F, T, V, I, ContainerTy> &IMP)
      : IMProblem(IMP), ICF(IMP.getICFG()) {}

  InterMonoSolver(const InterMonoSolver &) = delete;

  InterMonoSolver &operator=(const InterMonoSolver &) = delete;

  InterMonoSolver(InterMonoSolver &&) = delete;

  InterMonoSolver &operator=(InterMonoSolver &&) = delete;

  virtual ~InterMonoSolver() = default;

  std::unordered_map<N, std::unordered_map<CallStringCTX<N, K>, ContainerTy>>
  getAnalysis() {
    return Analysis;
  }

  virtual void solve() {
    initialize();
    while (!Worklist.empty()) {
      std::pair<N, N> Edge = Worklist.front();
      Worklist.pop_front();
      auto Src = Edge.first;
      auto Dst = Edge.second;
      if (ICF->isCallStmt(Src)) {
        addCalleesToWorklist(Edge);
      }
      // Compute the data-flow facts using the respective flow function
      std::unordered_map<CallStringCTX<N, K>, ContainerTy> Out;
      if (ICF->isCallStmt(Src)) {
        // Handle call and call-to-ret flow
        if (!isIntraEdge(Edge)) {
          // Handle call flow
          for (auto &[Ctx, Facts] : Analysis[Src]) {
            auto CTXAdd(Ctx);
            CTXAdd.push_back(Src);
            Out[CTXAdd] = IMProblem.callFlow(Src, ICF->getFunctionOf(Dst),
                                             Analysis[Src][Ctx]);
            bool FlowFactStabilized =
                IMProblem.equal_to(Out[CTXAdd], Analysis[Dst][CTXAdd]);
            if (!FlowFactStabilized) {
              Analysis[Dst][CTXAdd] =
                  IMProblem.merge(Analysis[Dst][CTXAdd], Out[CTXAdd]);
              addToWorklist({Src, Dst});
            }
          }
        } else {
          // Handle call-to-ret flow
          for (auto &[Ctx, Facts] : Analysis[Src]) {
            // call-to-ret flow does not modify contexts
            Out[Ctx] = IMProblem.callToRetFlow(
                Src, Dst, ICF->getCalleesOfCallAt(Src), Analysis[Src][Ctx]);
            bool FlowFactStabilized =
                IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
            if (!FlowFactStabilized) {
              Analysis[Dst][Ctx] =
                  IMProblem.merge(Analysis[Dst][Ctx], Out[Ctx]);
              addToWorklist({Src, Dst});
            }
          }
        }
      } else if (ICF->isExitStmt(Src)) {
        // Handle return flow
        for (auto &[Ctx, Facts] : Analysis[Src]) {
          auto CTXRm(Ctx);
          // we need to use several call- and retsites if the context is empty
          std::set<N> CallSites;
          std::set<N> RetSites;
          std::cout << "Ctx: " << Ctx << '\n';
          // handle empty context
          if (Ctx.empty()) {
            CallSites = ICF->getCallersOf(ICF->getFunctionOf(Src));
          } else {
            // handle context containing at least one element
            CallSites.insert(CTXRm.pop_back());
          }
          std::cout << "CTXRm: " << CTXRm << std::endl;
          // retrieve the possible return sites for each call
          for (auto CallSite : CallSites) {
            auto RetSitesPerCall = ICF->getReturnSitesOfCallAt(CallSite);
            RetSites.insert(RetSitesPerCall.begin(), RetSitesPerCall.end());
          }
          for (auto CallSite : CallSites) {
            auto RetFactsPerCall =
                IMProblem.returnFlow(CallSite, ICF->getFunctionOf(Src), Src,
                                     Dst, Analysis[Src][Ctx]);
            Out[CTXRm].insert(RetFactsPerCall.begin(), RetFactsPerCall.end());
          }
          for (auto RetSite : RetSites) {
            bool FlowFactStabilized =
                IMProblem.equal_to(Out[CTXRm], Analysis[RetSite][CTXRm]);
            if (!FlowFactStabilized) {
              Analysis[Dst][CTXRm] =
                  IMProblem.merge(Analysis[RetSite][CTXRm], Out[CTXRm]);
              addToWorklist({Src, RetSite});
            }
          }
        }
      } else {
        // Handle normal flow
        for (auto &[Ctx, Facts] : Analysis[Src]) {
          Out[Ctx] = IMProblem.normalFlow(Src, Analysis[Src][Ctx]);
          // need to merge if Dst is a branch target
          if (ICF->isBranchTarget(Src, Dst)) {
            for (auto Pred : ICF->getPredsOf(Dst)) {
              if (Pred != Src) {
                // we need to compute the out set of Pred and merge it with the
                // out set of Src on-the-fly as we do not have a dedicated
                // storage for merge points (otherwise we run into trouble with
                // merge operator such as set union)
                auto OtherPredOut =
                    IMProblem.normalFlow(Pred, Analysis[Pred][Ctx]);
                Out[Ctx] = IMProblem.merge(Out[Ctx], OtherPredOut);
              }
            }
          }
          // Check if data-flow facts have changed and if so, add Edge(s) to
          // worklist again.
          bool FlowFactStabilized =
              IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
          if (!FlowFactStabilized) {
            Analysis[Dst][Ctx] = Out[Ctx];
            addToWorklist({Src, Dst});
          }
        }
      }
    }
  }

  ContainerTy getResultsAt(N n) {
    ContainerTy Result;
    for (auto &[Ctx, Facts] : Analysis[n]) {
      Result.insert(Facts.begin(), Facts.end());
    }
    return Result;
  }

  virtual void dumpResults(std::ostream &OS = std::cout) {
    OS << "======= DUMP LLVM-INTER-MONOTONE-SOLVER RESULTS =======\n";
    for (auto &[Node, ContextMap] : this->Analysis) {
      OS << "Instruction:\n" << this->IMProblem.NtoString(Node);
      OS << "\nFacts:\n";
      if (ContextMap.empty()) {
        OS << "\tEMPTY\n";
      } else {
        for (auto &[Context, FlowFacts] : ContextMap) {
          OS << Context << '\n';
          if (FlowFacts.empty()) {
            OS << "\tEMPTY\n";
          } else {
            for (auto FlowFact : FlowFacts) {
              OS << this->IMProblem.DtoString(FlowFact);
            }
          }
        }
      }
      OS << '\n';
    }
  }

  virtual void emitTextReport(std::ostream &OS = std::cout) {}

  virtual void emitGraphicalReport(std::ostream &OS = std::cout) {}
};

template <typename Problem, unsigned K>
using InterMonoSolver_P =
    InterMonoSolver<typename Problem::n_t, typename Problem::d_t,
                    typename Problem::f_t, typename Problem::t_t,
                    typename Problem::v_t, typename Problem::i_t,
                    typename Problem::container_t, K>;

} // namespace psr

#endif
