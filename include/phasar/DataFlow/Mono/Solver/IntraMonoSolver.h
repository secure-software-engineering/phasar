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

#ifndef PHASAR_DATAFLOW_MONO_SOLVER_INTRAMONOSOLVER_H
#define PHASAR_DATAFLOW_MONO_SOLVER_INTRAMONOSOLVER_H

#include "phasar/DataFlow/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/Utilities.h"

#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>

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
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

protected:
  ProblemTy &IMProblem;
  std::deque<std::pair<n_t, n_t>> Worklist;
  std::unordered_map<n_t, mono_container_t> Analysis;
  const CFGBase<c_t> *CFG;

  void initialize() {
    const auto &EntryPoints = IMProblem.getEntryPoints();
    for (const auto &EntryPoint : EntryPoints) {
      auto Function =
          IMProblem.getProjectIRDB()->getFunctionDefinition(EntryPoint);
      auto ControlFlowEdges = CFG->getAllControlFlowEdges(Function);
      // add all intra-procedural edges to the worklist
      Worklist.insert(Worklist.begin(), ControlFlowEdges.begin(),
                      ControlFlowEdges.end());
      // set all analysis information to the empty set
      for (auto Insts : CFG->getAllInstructionsOf(Function)) {
        Analysis.insert(std::make_pair(Insts, IMProblem.allTop()));
      }
    }
    // insert initial seeds
    for (auto &[Node, FlowFacts] : IMProblem.initialSeeds()) {
      Analysis[Node].insert(FlowFacts.begin(), FlowFacts.end());
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
      // llvm::outs() << "worklist size: " << Worklist.size() << "\n";
      std::pair<n_t, n_t> Edge = Worklist.front();
      Worklist.pop_front();
      n_t Src = Edge.first;
      n_t Dst = Edge.second;
      auto Out = IMProblem.normalFlow(Src, Analysis[Src]);
      // need to merge if Dst is a branch target
      if (CFG->isBranchTarget(Src, Dst)) {
        for (auto Pred : CFG->getPredsOf(Dst)) {
          if (Pred != Src) {
            // we need to compute the out set of Pred and merge it with the out
            // set of Src on-the-fly as we do not have a dedicated storage for
            // merge points (otherwise we run into trouble with merge operator
            // such as set union)
            auto OtherPredOut = IMProblem.normalFlow(Pred, Analysis[Pred]);
            Out = IMProblem.merge(Out, OtherPredOut);
          }
        }
      }
      if (!IMProblem.equal_to(Out, Analysis[Dst])) {
        Analysis[Dst] = Out;
        for (auto Nprimeprime : CFG->getSuccsOf(Dst)) {
          Worklist.push_back({Dst, Nprimeprime});
        }
      }
    }
    // step 3: Presenting the result (MFP_in and MFP_out)
    // MFP_in[s] = Analysis[s];
    // MFP out[s] = IMProblem.flow(Analysis[s]);
    for (auto &[Node, FlowFacts] : Analysis) {
      FlowFacts = IMProblem.normalFlow(Node, FlowFacts);
    }
  }

  [[nodiscard]] const mono_container_t &getResultsAt(n_t Stmt) const {
    auto It = Analysis.find(Stmt);
    if (It != Analysis.end()) {
      return It->second;
    }
    return getDefaultValue<mono_container_t>();
  }

  virtual void dumpResults(llvm::raw_ostream &OS = llvm::outs()) {
    OS << "\n***************************************************************\n"
       << "*                 Raw IntraMonoSolver results                 *\n"
       << "***************************************************************\n";

    if (Analysis.empty()) {
      OS << "No results computed!" << '\n';
      return;
    }

    std::vector<std::pair<n_t, mono_container_t>> Cells;
    Cells.reserve(Analysis.size());
    Cells.insert(Cells.end(), Analysis.begin(), Analysis.end());

    std::sort(Cells.begin(), Cells.end(), [](const auto &Lhs, const auto &Rhs) {
      if constexpr (std::is_same_v<n_t, const llvm::Instruction *>) {
        return StringIDLess{}(getMetaDataID(Lhs.first),
                              getMetaDataID(Rhs.first));
      } else {
        // If non-LLVM IR is used
        return Lhs.first < Rhs.first;
      }
    });

    for (const auto &[Node, FlowFacts] : Cells) {
      OS << "Instruction: " << NToString(Node);
      OS << "\nFacts: ";
      if (FlowFacts.empty()) {
        OS << "EMPTY\n";
      } else {
        IMProblem.printContainer(OS, FlowFacts);
      }
      OS << "\n";
    }
  }

  virtual void emitTextReport(llvm::raw_ostream &OS = llvm::outs()) {}

  virtual void emitGraphicalReport(llvm::raw_ostream &OS = llvm::outs()) {}
};

template <typename Problem>
IntraMonoSolver(Problem &)
    -> IntraMonoSolver<typename Problem::ProblemAnalysisDomain>;

template <typename Problem>
using IntraMonoSolver_P =
    IntraMonoSolver<typename Problem::ProblemAnalysisDomain>;

} // namespace psr

#endif
