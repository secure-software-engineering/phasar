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

#ifndef PHASAR_DATAFLOW_MONO_SOLVER_INTERMONOSOLVER_H
#define PHASAR_DATAFLOW_MONO_SOLVER_INTERMONOSOLVER_H

#include "phasar/DataFlow/Mono/Contexts/CallStringCTX.h"
#include "phasar/DataFlow/Mono/InterMonoProblem.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace psr {

template <typename AnalysisDomainTy, unsigned K> class InterMonoSolver {
public:
  using ProblemTy = InterMonoProblem<AnalysisDomainTy>;
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

protected:
  ProblemTy &IMProblem;
  std::deque<std::pair<n_t, n_t>> Worklist;
  std::unordered_map<
      n_t, std::unordered_map<CallStringCTX<n_t, K>, mono_container_t>>
      Analysis;
  std::unordered_set<f_t> AddedFunctions;
  const i_t *ICF;

  void initialize() {
    for (auto &[Node, FlowFacts] : IMProblem.initialSeeds()) {
      auto ControlFlowEdges =
          ICF->getAllControlFlowEdges(ICF->getFunctionOf(Node));
      Worklist.insert(Worklist.begin(), ControlFlowEdges.begin(),
                      ControlFlowEdges.end());
      // Initialize with empty context and empty data-flow set such that the
      // flow functions are at least called once per instruction
      for (auto &[Src, Dst] : ControlFlowEdges) {
        Analysis[Src][CallStringCTX<n_t, K>()] = IMProblem.allTop();
      }
      // Initialize last
      if (!ControlFlowEdges.empty()) {
        Analysis[ControlFlowEdges.back().second][CallStringCTX<n_t, K>()] =
            IMProblem.allTop();
      }
      // Additionally, insert the initial seeds
      Analysis[Node][CallStringCTX<n_t, K>()].insert(FlowFacts.begin(),
                                                     FlowFacts.end());
    }
  }

  bool isIntraEdge(std::pair<n_t, n_t> Edge) {
    return ICF->getFunctionOf(Edge.first) == ICF->getFunctionOf(Edge.second);
  }

  bool isCallEdge(std::pair<n_t, n_t> Edge) {
    return !isIntraEdge(Edge) && ICF->isCallSite(Edge.first);
  }

  bool isReturnEdge(std::pair<n_t, n_t> Edge) {
    return !isIntraEdge(Edge) && ICF->isExitInst(Edge.first);
  }

  void printWorkList() {
    llvm::outs() << "CURRENT WORKLIST:\n";
    for (auto &[Src, Dst] : Worklist) {
      llvm::outs() << NToString(Src) << " --> " << NToString(Dst) << '\n';
    }
    llvm::outs() << "-----------------\n";
  }

  void addCalleesToWorklist(std::pair<n_t, n_t> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    // Add inter- and intra-edges of callee(s)
    for (auto Callee : ICF->getCalleesOfCallAt(Src)) {
      if (AddedFunctions.find(Callee) != AddedFunctions.end()) {
        break;
      }
      AddedFunctions.insert(Callee);
      // Add call Edge(s)
      for (auto StartPoint : ICF->getStartPointsOf(Callee)) {
        Worklist.push_back({Src, StartPoint});
      }
      // Add intra edges of callee
      auto Edges = ICF->getAllControlFlowEdges(Callee);
      Worklist.insert(Worklist.begin(), Edges.begin(), Edges.end());
      // Initialize with empty context and empty data-flow set such that the
      // flow functions are at least called once per instruction
      for (auto &[Src, Dst] : Edges) {
        Analysis[Src][CallStringCTX<n_t, K>()] = IMProblem.allTop();
      }
      // Initialize last
      if (!Edges.empty()) {
        Analysis[Edges.back().second][CallStringCTX<n_t, K>()] =
            IMProblem.allTop();
      }
      // Add return Edge(s)
      for (auto Ret : ICF->getExitPointsOf(Callee)) {
        for (auto RetSite : ICF->getReturnSitesOfCallAt(Src)) {
          Worklist.push_back({Ret, RetSite});
        }
      }
    }
    // The (intra-procedural) call-to-return Edge of the caller is already in
    // the worklist
  }

  void addToWorklist(std::pair<n_t, n_t> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    Worklist.push_back({Src, Dst});
    // add intra-procedural edges again
    for (auto Nprimeprime : ICF->getSuccsOf(Dst)) {
      Worklist.push_back({Dst, Nprimeprime});
    }
    // add inter-procedural call edges again
    if (ICF->isCallSite(Dst)) {
      for (auto Callee : ICF->getCalleesOfCallAt(Dst)) {
        for (auto StartPoint : ICF->getStartPointsOf(Callee)) {
          Worklist.push_back({Dst, StartPoint});
        }
      }
    }
    // add inter-procedural return edges again
    if (ICF->isExitInst(Dst)) {
      for (const auto *Caller : ICF->getCallersOf(ICF->getFunctionOf(Dst))) {
        for (const auto *Nprimeprime : ICF->getSuccsOf(Caller)) {
          Worklist.push_back({Dst, Nprimeprime});
        }
      }
    }
  }

public:
  InterMonoSolver(InterMonoProblem<AnalysisDomainTy> &IMP)
      : IMProblem(IMP), ICF(IMP.getICFG()) {}

  InterMonoSolver(const InterMonoSolver &) = delete;

  InterMonoSolver &operator=(const InterMonoSolver &) = delete;

  InterMonoSolver(InterMonoSolver &&) = delete;

  InterMonoSolver &operator=(InterMonoSolver &&) = delete;

  virtual ~InterMonoSolver() = default;

  std::unordered_map<
      n_t, std::unordered_map<CallStringCTX<n_t, K>, mono_container_t>>
  getAnalysis() {
    return Analysis;
  }

  void processNormal(std::pair<n_t, n_t> Edge) {
    llvm::outs() << "Handle normal flow\n";
    auto Src = Edge.first;
    auto Dst = Edge.second;
    llvm::outs() << "Src: " << NToString(Src) << '\n';
    llvm::outs() << "Dst: " << NToString(Dst) << '\n';
    std::unordered_map<CallStringCTX<n_t, K>, mono_container_t> Out;
    for (auto &[Ctx, Facts] : Analysis[Src]) {
      Out[Ctx] = IMProblem.normalFlow(Src, Analysis[Src][Ctx]);
      // need to merge if Dst is a branch target
      if (ICF->isBranchTarget(Src, Dst)) {
        llvm::outs() << "Num preds: " << ICF->getPredsOf(Dst).size() << '\n';
        for (auto Pred : ICF->getPredsOf(Dst)) {
          if (Pred != Src) {
            // we need to compute the out set of Pred and merge it with the
            // out set of Src on-the-fly as we do not have a dedicated
            // storage for merge points (otherwise we run into trouble with
            // merge operator such as set union)
            auto OtherPredOut = IMProblem.normalFlow(Pred, Analysis[Pred][Ctx]);
            Out[Ctx] = IMProblem.merge(Out[Ctx], OtherPredOut);
          }
        }
      }
      // Check if data-flow facts have changed and if so, add Edge(s) to
      // worklist again.
      llvm::outs() << "\nNormal Out[Ctx]:\n";
      IMProblem.printContainer(llvm::outs(), Out[Ctx]);
      llvm::outs() << "\nAnalysis[Dst][Ctx]:\n";
      IMProblem.printContainer(llvm::outs(), Analysis[Dst][Ctx]);
      bool FlowFactStabilized =
          IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
      if (!FlowFactStabilized) {
        llvm::outs() << "\nNormal stabilized? --> " << FlowFactStabilized
                     << '\n';
        auto Merged = Out[Ctx];
        llvm::outs() << "Normal merged:\n";
        IMProblem.printContainer(llvm::outs(), Merged);
        llvm::outs() << '\n';
        Analysis[Dst][Ctx] = Merged;
        addToWorklist({Src, Dst});
      }
    }
  }

  void processCall(std::pair<n_t, n_t> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    std::unordered_map<CallStringCTX<n_t, K>, mono_container_t> Out;
    if (!isIntraEdge(Edge)) {
      llvm::outs() << "Handle call flow\n";
      llvm::outs() << "Src: " << NToString(Src) << '\n';
      llvm::outs() << "Dst: " << NToString(Dst) << '\n';
      for (auto &[Ctx, Facts] : Analysis[Src]) {
        auto CTXAdd(Ctx);
        CTXAdd.push_back(Src);
        Out[CTXAdd] = IMProblem.callFlow(Src, ICF->getFunctionOf(Dst),
                                         Analysis[Src][Ctx]);
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[CTXAdd], Analysis[Dst][CTXAdd]);
        llvm::outs() << "Call Out[CTXAdd]:\n";
        IMProblem.printContainer(llvm::outs(), Out[CTXAdd]);
        llvm::outs() << '\n';
        llvm::outs() << "Call Analysis[Dst][CTXAdd]:\n";
        IMProblem.printContainer(llvm::outs(), Analysis[Dst][CTXAdd]);
        llvm::outs() << '\n';
        llvm::outs() << "Call stabilized? --> " << FlowFactStabilized << '\n';
        if (!FlowFactStabilized) {
          // auto merge = IMProblem.merge(Analysis[Dst][CTXAdd], Out[CTXAdd]);
          auto Merge = Out[CTXAdd];
          llvm::outs() << "Call merge:\n";
          IMProblem.printContainer(llvm::outs(), Merge);
          llvm::outs() << '\n';
          Analysis[Dst][CTXAdd] = Merge;
          addToWorklist({Src, Dst});
        }
      }
    } else {
      // Handle call-to-ret flow
      llvm::outs() << "Handle call to ret flow\n";
      llvm::outs() << "Src: " << NToString(Src) << '\n';
      llvm::outs() << "Dst: " << NToString(Dst) << '\n';
      for (auto &[Ctx, Facts] : Analysis[Src]) {
        // call-to-ret flow does not modify contexts
        Out[Ctx] = IMProblem.callToRetFlow(
            Src, Dst, ICF->getCalleesOfCallAt(Src), Analysis[Src][Ctx]);
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
        llvm::outs() << "Call to ret stabilized? --> " << FlowFactStabilized
                     << '\n';
        llvm::outs() << "Call Out[Ctx]:\n";
        IMProblem.printContainer(llvm::outs(), Out[Ctx]);
        llvm::outs() << '\n';
        llvm::outs() << "Call Analysis[Dst][CTX]:\n";
        IMProblem.printContainer(llvm::outs(), Analysis[Dst][Ctx]);
        llvm::outs() << '\n';
        if (!FlowFactStabilized) {
          auto Merge = Out[Ctx];
          llvm::outs() << "Call to ret merge:\n";
          IMProblem.printContainer(llvm::outs(), Merge);
          llvm::outs() << '\n';
          Analysis[Dst][Ctx] =
              Merge; // IMProblem.merge(Analysis[Dst][Ctx], Out[Ctx]);
          addToWorklist({Src, Dst});
        }
      }
    }
  }

  void processExit(std::pair<n_t, n_t> Edge) {
    auto Src = Edge.first;
    auto Dst = Edge.second;
    std::unordered_map<CallStringCTX<n_t, K>, mono_container_t> Out;
    llvm::outs() << "\nHandle ret flow in: "
                 << ICF->getFunctionName(ICF->getFunctionOf(Src)) << '\n';
    llvm::outs() << "Src: " << NToString(Src) << '\n';
    llvm::outs() << "Dst: " << NToString(Dst) << '\n';
    for (auto &[Ctx, Facts] : Analysis[Src]) {
      auto CTXRm(Ctx);
      CTXRm.print(llvm::outs() << "CTXRm: ") << '\n';
      // we need to use several call- and retsites if the context is empty
      llvm::SmallVector<n_t> CallSites;

      // handle empty context
      if (Ctx.empty()) {
        const auto &Callers = ICF->getCallersOf(ICF->getFunctionOf(Src));
        CallSites.append(Callers.begin(), Callers.end());
      } else {
        // handle context containing at least one element
        CallSites.push_back(CTXRm.pop_back());
      }

      std::set<n_t> RetSites;
      // retrieve the possible return sites for each call
      for (auto CallSite : CallSites) {
        auto RetSitesPerCall = ICF->getReturnSitesOfCallAt(CallSite);
        RetSites.insert(RetSitesPerCall.begin(), RetSitesPerCall.end());
      }
      for (auto CallSite : CallSites) {
        auto RetFactsPerCall = IMProblem.returnFlow(
            CallSite, ICF->getFunctionOf(Src), Src, Dst, Analysis[Src][Ctx]);
        Out[CTXRm].insert(RetFactsPerCall.begin(), RetFactsPerCall.end());
      }
      // TODO!
      llvm::outs() << "ResSites.size(): " << RetSites.size() << '\n';
      for (auto RetSite : RetSites) {
        llvm::outs() << "RetSite: " << NToString(RetSite) << '\n';
        llvm::outs() << "Return facts: ";
        IMProblem.printContainer(llvm::outs(), Out[CTXRm]);
        llvm::outs() << '\n';
        llvm::outs() << "RetSite facts: ";
        IMProblem.printContainer(llvm::outs(), Analysis[RetSite][CTXRm]);
        llvm::outs() << '\n';
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[CTXRm], Analysis[RetSite][CTXRm]);
        llvm::outs() << "Ret stabilized? --> " << FlowFactStabilized << '\n';
        if (!FlowFactStabilized) {
          mono_container_t Merge;
          Merge.insert(Analysis[RetSite][CTXRm].begin(),
                       Analysis[RetSite][CTXRm].end());
          Merge.insert(Out[CTXRm].begin(), Out[CTXRm].end());
          Analysis[RetSite][CTXRm] = Merge;
          Analysis[Dst][CTXRm] = Merge;
          // IMProblem.merge(Analysis[RetSite][CTXRm], Out[CTXRm]);
          llvm::outs() << "Merged to: ";
          IMProblem.printContainer(llvm::outs(), Merge);
          llvm::outs() << '\n';
          // addToWorklist({Src, RetSite});
        }
      }
    }
  }

  mono_container_t
  summarize(/*Function CalleeTarget, mono_container_t FactAtCall */) {
    // spawn a new analysis with its own worklist
  }

  bool isSensibleToSummarize() {
    // use a heuristic to check whether we should compute a summary
    // make use of the call-graph information
    return false;
  }

  virtual void solve() {
    initialize();
    while (!Worklist.empty()) {
      std::pair<n_t, n_t> Edge = Worklist.front();
      Worklist.pop_front();
      auto Src = Edge.first;
      auto Dst = Edge.second;
      if (ICF->isCallSite(Src)) {
        addCalleesToWorklist(Edge);
      }
      // Compute the data-flow facts using the respective kind of flows
      if (ICF->isCallSite(Src)) {
        // Handle call flow(s)
        if (!isIntraEdge(Edge)) {
          // real call
          for (auto &[Ctx, Facts] : Analysis[Src]) {
            processCall(Edge); // TODO: decompose into processCall and
                               // processCallToRet
          }
        } else {
          // call-to-return
          processCall(
              Edge); // TODO: decompose into processCall and processCallToRet
        }
      } else if (ICF->isExitInst(Src)) {
        // Handle return flow
        processExit(Edge);
      } else {
        // Handle normal flow
        processNormal(Edge);
      }
    }
  }

  mono_container_t getResultsAt(n_t Stmt) {
    mono_container_t Result;
    for (auto &[Ctx, Facts] : Analysis[Stmt]) {
      Result.insert(Facts.begin(), Facts.end());
    }
    return Result;
  }

  virtual void dumpResults(llvm::raw_ostream &OS = llvm::outs()) {
    OS << "======= DUMP LLVM-INTER-MONOTONE-SOLVER RESULTS =======\n";
    for (auto &[Node, ContextMap] : this->Analysis) {
      OS << "Instruction:\n" << NToString(Node);
      OS << "\nFacts:\n";
      if (ContextMap.empty()) {
        OS << "\tEMPTY\n";
      } else {
        for (auto &[Context, FlowFacts] : ContextMap) {
          Context.print(OS) << '\n';
          if (FlowFacts.empty()) {
            OS << "\tEMPTY\n";
          } else {
            IMProblem.printContainer(OS, FlowFacts);
          }
        }
      }
      OS << '\n';
    }
  }

  virtual void emitTextReport(llvm::raw_ostream &OS = llvm::outs()) {}

  virtual void emitGraphicalReport(llvm::raw_ostream &OS = llvm::outs()) {}
};

template <typename Problem, unsigned K>
using InterMonoSolver_P =
    InterMonoSolver<typename Problem::ProblemAnalysisDomain, K>;

} // namespace psr

#endif
