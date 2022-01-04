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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_SOLVER_INTERMONOSOLVER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_SOLVER_INTERMONOSOLVER_H

#include <deque>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Contexts/CallStringCTX.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/Utils/LLVMShorthands.h"

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
    std::cout << "CURRENT WORKLIST:\n";
    for (auto &[Src, Dst] : Worklist) {
      std::cout << llvmIRToString(Src) << " --> " << llvmIRToString(Dst)
                << '\n';
    }
    std::cout << "-----------------\n";
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
    std::cout << "Handle normal flow\n";
    auto Src = Edge.first;
    auto Dst = Edge.second;
    std::cout << "Src: " << llvmIRToString(Src) << '\n';
    std::cout << "Dst: " << llvmIRToString(Dst) << '\n';
    std::unordered_map<CallStringCTX<n_t, K>, mono_container_t> Out;
    for (auto &[Ctx, Facts] : Analysis[Src]) {
      Out[Ctx] = IMProblem.normalFlow(Src, Analysis[Src][Ctx]);
      // need to merge if Dst is a branch target
      if (ICF->isBranchTarget(Src, Dst)) {
        std::cout << "Num preds: " << ICF->getPredsOf(Dst).size() << '\n';
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
      std::cout << "\nNormal Out[Ctx]:\n";
      IMProblem.printContainer(std::cout, Out[Ctx]);
      std::cout << "\nAnalysis[Dst][Ctx]:\n";
      IMProblem.printContainer(std::cout, Analysis[Dst][Ctx]);
      bool FlowFactStabilized =
          IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
      if (!FlowFactStabilized) {
        std::cout << "\nNormal stabilized? --> " << FlowFactStabilized << '\n';
        auto Merged = Out[Ctx];
        std::cout << "Normal merged:\n";
        IMProblem.printContainer(std::cout, Merged);
        std::cout << '\n';
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
      std::cout << "Handle call flow\n";
      std::cout << "Src: " << llvmIRToString(Src) << '\n';
      std::cout << "Dst: " << llvmIRToString(Dst) << '\n';
      for (auto &[Ctx, Facts] : Analysis[Src]) {
        auto CTXAdd(Ctx);
        CTXAdd.push_back(Src);
        Out[CTXAdd] = IMProblem.callFlow(Src, ICF->getFunctionOf(Dst),
                                         Analysis[Src][Ctx]);
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[CTXAdd], Analysis[Dst][CTXAdd]);
        std::cout << "Call Out[CTXAdd]:\n";
        IMProblem.printContainer(std::cout, Out[CTXAdd]);
        std::cout << '\n';
        std::cout << "Call Analysis[Dst][CTXAdd]:\n";
        IMProblem.printContainer(std::cout, Analysis[Dst][CTXAdd]);
        std::cout << '\n';
        std::cout << "Call stabilized? --> " << FlowFactStabilized << '\n';
        if (!FlowFactStabilized) {
          // auto merge = IMProblem.merge(Analysis[Dst][CTXAdd], Out[CTXAdd]);
          auto Merge = Out[CTXAdd];
          std::cout << "Call merge:\n";
          IMProblem.printContainer(std::cout, Merge);
          std::cout << '\n';
          Analysis[Dst][CTXAdd] = Merge;
          addToWorklist({Src, Dst});
        }
      }
    } else {
      // Handle call-to-ret flow
      std::cout << "Handle call to ret flow\n";
      std::cout << "Src: " << llvmIRToString(Src) << '\n';
      std::cout << "Dst: " << llvmIRToString(Dst) << '\n';
      for (auto &[Ctx, Facts] : Analysis[Src]) {
        // call-to-ret flow does not modify contexts
        Out[Ctx] = IMProblem.callToRetFlow(
            Src, Dst, ICF->getCalleesOfCallAt(Src), Analysis[Src][Ctx]);
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[Ctx], Analysis[Dst][Ctx]);
        std::cout << "Call to ret stabilized? --> " << FlowFactStabilized
                  << '\n';
        std::cout << "Call Out[Ctx]:\n";
        IMProblem.printContainer(std::cout, Out[Ctx]);
        std::cout << '\n';
        std::cout << "Call Analysis[Dst][CTX]:\n";
        IMProblem.printContainer(std::cout, Analysis[Dst][Ctx]);
        std::cout << '\n';
        if (!FlowFactStabilized) {
          auto Merge = Out[Ctx];
          std::cout << "Call to ret merge:\n";
          IMProblem.printContainer(std::cout, Merge);
          std::cout << '\n';
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
    std::cout << "\nHandle ret flow in: "
              << ICF->getFunctionName(ICF->getFunctionOf(Src)) << '\n';
    std::cout << "Src: " << llvmIRToString(Src) << '\n';
    std::cout << "Dst: " << llvmIRToString(Dst) << '\n';
    for (auto &[Ctx, Facts] : Analysis[Src]) {
      auto CTXRm(Ctx);
      std::cout << "CTXRm: " << CTXRm << '\n';
      // we need to use several call- and retsites if the context is empty
      std::set<n_t> CallSites;
      std::set<n_t> RetSites;
      // handle empty context
      if (Ctx.empty()) {
        CallSites = ICF->getCallersOf(ICF->getFunctionOf(Src));
      } else {
        // handle context containing at least one element
        CallSites.insert(CTXRm.pop_back());
      }
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
      std::cout << "ResSites.size(): " << RetSites.size() << '\n';
      for (auto RetSite : RetSites) {
        std::cout << "RetSite: " << llvmIRToString(RetSite) << '\n';
        std::cout << "Return facts: ";
        IMProblem.printContainer(std::cout, Out[CTXRm]);
        std::cout << '\n';
        std::cout << "RetSite facts: ";
        IMProblem.printContainer(std::cout, Analysis[RetSite][CTXRm]);
        std::cout << '\n';
        bool FlowFactStabilized =
            IMProblem.equal_to(Out[CTXRm], Analysis[RetSite][CTXRm]);
        std::cout << "Ret stabilized? --> " << FlowFactStabilized << '\n';
        if (!FlowFactStabilized) {
          mono_container_t Merge;
          Merge.insert(Analysis[RetSite][CTXRm].begin(),
                       Analysis[RetSite][CTXRm].end());
          Merge.insert(Out[CTXRm].begin(), Out[CTXRm].end());
          Analysis[RetSite][CTXRm] = Merge;
          Analysis[Dst][CTXRm] = Merge;
          // IMProblem.merge(Analysis[RetSite][CTXRm], Out[CTXRm]);
          std::cout << "Merged to: ";
          IMProblem.printContainer(std::cout, Merge);
          std::cout << '\n';
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
    InterMonoSolver<typename Problem::ProblemAnalysisDomain, K>;

} // namespace psr

#endif
