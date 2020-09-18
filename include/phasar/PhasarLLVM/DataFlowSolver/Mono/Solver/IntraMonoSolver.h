/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoSolver.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_SOLVER_INTRAMONOSOLVER_H_
#define PHASAR_PHASARLLVM_MONO_SOLVER_INTRAMONOSOLVER_H_

#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/Utils/BitVectorSet.h"

namespace psr {

template <typename AnalysisDomainTy> class IntraMonoSolver {
public:
  using ProblemTy = IntraMonoProblem<AnalysisDomainTy>;
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using c_t = typename AnalysisDomainTy::c_t;

protected:
  ProblemTy &IMProblem;
  std::deque<std::pair<n_t, n_t>> Worklist;
  std::unordered_map<n_t, BitVectorSet<d_t>> Analysis;
  const c_t *CFG;

  void initialize() {
    auto EntryPoints = IMProblem.getEntryPoints();
    for (const auto &EntryPoint : EntryPoints) {
      auto Function =
          IMProblem.getProjectIRDB()->getFunctionDefinition(EntryPoint);
      auto ControlFlowEdges = CFG->getAllControlFlowEdges(Function);
      // add all intra-procedural edges to the worklist
      Worklist.insert(Worklist.begin(), ControlFlowEdges.begin(),
                      ControlFlowEdges.end());
      // set all analysis information to the empty set
      for (auto s : CFG->getAllInstructionsOf(Function)) {
        Analysis.insert(std::make_pair(s, BitVectorSet<d_t>()));
      }
    }
    // insert initial seeds
    for (auto &[Node, FlowFacts] : IMProblem.initialSeeds()) {
      Analysis[Node].insert(FlowFacts);
    }
  }

public:
  IntraMonoSolver(ProblemTy &IMP) : IMProblem(IMP), CFG(IMP.getCFG()) {}
  virtual ~IntraMonoSolver() = default;
  virtual void solve() {
    // step 1: Initalization (of Worklist and Analysis)
    initialize();
    // step 2: Iteration (updating Worklist and Analysis)
    while (!Worklist.empty()) {
      // std::cout << "worklist size: " << Worklist.size() << "\n";
      std::pair<n_t, n_t> path = Worklist.front();
      Worklist.pop_front();
      n_t src = path.first;
      n_t dst = path.second;
      BitVectorSet<d_t> Out = IMProblem.normalFlow(src, Analysis[src]);
      if (!IMProblem.sqSubSetEqual(Out, Analysis[dst])) {
        Analysis[dst] = IMProblem.join(Analysis[dst], Out);
        for (auto nprimeprime : CFG->getSuccsOf(dst)) {
          Worklist.push_back({dst, nprimeprime});
        }
      }
    }
    // step 3: Presenting the result (MFP_in and MFP_out)
    // MFP_in[s] = Analysis[s];
    // MFP out[s] = IMProblem.flow(Analysis[s]);
    for (auto entry : Analysis) {
      entry.second = IMProblem.normalFlow(entry.first, entry.second);
    }
  }

  BitVectorSet<d_t> getResultsAt(n_t n) { return Analysis[n]; }

  virtual void dumpResults(std::ostream &OS = std::cout) {
    OS << "Intra-Monotone solver results:\n"
          "------------------------------\n";
    for (auto &[Node, FlowFacts] : this->Analysis) {
      OS << "Instruction:\n" << this->IMProblem.NtoString(Node);
      OS << "\nFacts:\n";
      if (FlowFacts.empty()) {
        OS << "\tEMPTY\n";
      } else {
        for (auto FlowFact : FlowFacts) {
          OS << this->IMProblem.DtoString(FlowFact) << '\n';
        }
      }
      OS << "\n\n";
    }
  }

  virtual void emitTextReport(std::ostream &OS = std::cout) {}

  virtual void emitGraphicalReport(std::ostream &OS = std::cout) {}
};

template <typename Problem>
IntraMonoSolver(Problem &)
    -> IntraMonoSolver<typename Problem::ProblemAnalysisDomain>;

template <typename Problem>
using IntraMonoSolver_P =
    IntraMonoSolver<typename Problem::ProblemAnalysisDomain>;

} // namespace psr

#endif
