/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * SolverResults.h
 *
 *  Created on: 19.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_SOLVERRESULTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_SOLVERRESULTS_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Table.h"
#include "phasar/Utils/Utilities.h"

#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace llvm {
class Instruction;
class Value;
} // namespace llvm

namespace psr {
// For sorting the results in dumpResults()
std::string getMetaDataID(const llvm::Value *V);

template <typename N, typename D, typename L> class SolverResults {
public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  SolverResults(Table<N, D, L> &ResTab,
                D ZV) noexcept(std::is_nothrow_move_constructible_v<D>)
      : Results(ResTab), ZV(std::move(ZV)) {}
  SolverResults(Table<N, D, L> &&ResTab, D ZV) = delete;

  [[nodiscard]] L resultAt(ByConstRef<N> Stmt, ByConstRef<D> Node) const {
    return Results.get(Stmt, Node);
  }

  [[nodiscard]] std::unordered_map<D, L>
  resultsAt(ByConstRef<N> Stmt, bool StripZero = false) const {
    std::unordered_map<D, L> Result = Results.row(Stmt);
    if (StripZero) {
      Result.erase(ZV);
    }
    return Result;
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  template <typename ValueDomain = L,
            typename = typename std::enable_if_t<
                std::is_same_v<ValueDomain, BinaryDomain>>>
  [[nodiscard]] std::set<D> ifdsResultsAt(ByConstRef<N> Stmt) const {
    std::set<D> KeySet;
    std::unordered_map<D, BinaryDomain> ResultMap = this->resultsAt(Stmt);
    for (auto FlowFact : ResultMap) {
      KeySet.insert(FlowFact.first);
    }
    return KeySet;
  }

  [[nodiscard]] std::vector<typename Table<N, D, L>::Cell>
  getAllResultEntries() const {
    return Results.cellVec();
  }

  template <typename ICFGTy>
  void dumpResults(const ICFGTy &ICF, const NodePrinterBase<n_t> &NP,
                   const DataFlowFactPrinterBase<d_t> &DP,
                   const EdgeFactPrinterBase<l_t> &LP,
                   llvm::raw_ostream &OS = llvm::outs()) {
    using f_t = typename ICFGTy::f_t;

    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    OS << "\n***************************************************************\n"
       << "*                  Raw IDESolver results                      *\n"
       << "***************************************************************\n";
    auto Cells = Results.cellVec();
    if (Cells.empty()) {
      OS << "No results computed!" << '\n';
    } else {
      std::sort(
          Cells.begin(), Cells.end(), [](const auto &Lhs, const auto &Rhs) {
            if constexpr (std::is_same_v<n_t, const llvm::Instruction *>) {
              return StringIDLess{}(getMetaDataID(Lhs.getRowKey()),
                                    getMetaDataID(Rhs.getRowKey()));
            } else {
              // If non-LLVM IR is used
              return Lhs.getRowKey() < Rhs.getRowKey();
            }
          });
      n_t Prev = n_t{};
      n_t Curr = n_t{};
      f_t PrevFn = f_t{};
      f_t CurrFn = f_t{};
      for (unsigned I = 0; I < Cells.size(); ++I) {
        Curr = Cells[I].getRowKey();
        CurrFn = ICF.getFunctionOf(Curr);
        if (PrevFn != CurrFn) {
          PrevFn = CurrFn;
          OS << "\n\n============ Results for function '" +
                    ICF.getFunctionName(CurrFn) + "' ============\n";
        }
        if (Prev != Curr) {
          Prev = Curr;
          std::string NString = NP.NtoString(Curr);
          std::string Line(NString.size(), '-');
          OS << "\n\nN: " << NString << "\n---" << Line << '\n';
        }
        OS << "\tD: " << DP.DtoString(Cells[I].getColumnKey())
           << " | V: " << LP.LtoString(Cells[I].getValue()) << '\n';
      }
    }
    OS << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  template <typename ICFGTy, typename ProblemTy>
  void dumpResults(const ICFGTy &ICF, const ProblemTy &IDEProblem,
                   llvm::raw_ostream &OS = llvm::outs()) {
    dumpResults(ICF, IDEProblem, IDEProblem, IDEProblem, OS);
  }

private:
  Table<N, D, L> &Results;
  D ZV;
};

template <typename N, typename D, typename L> class OwningSolverResults {
public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  OwningSolverResults(Table<N, D, L> ResTab,
                      D ZV) noexcept(std::is_nothrow_move_constructible_v<D>)
      : Results(std::move(ResTab)), ZV(ZV) {}

  [[nodiscard]] operator SolverResults<N, D, L>() const &noexcept(
      std::is_nothrow_copy_constructible_v<D>) {
    return {Results, ZV};
  }
  operator SolverResults<N, D, L>() && = delete;

  [[nodiscard]] ByConstRef<L> resultAt(ByConstRef<N> Stmt,
                                       ByConstRef<D> Node) const {
    return Results.get(Stmt, Node);
  }

  [[nodiscard]] std::unordered_map<D, L>
  resultsAt(ByConstRef<N> Stmt, bool StripZero = false) const {
    return static_cast<SolverResults<N, D, L>>(*this).resultsAt(Stmt,
                                                                StripZero);
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  template <typename ValueDomain = L,
            typename = typename std::enable_if_t<
                std::is_same_v<ValueDomain, BinaryDomain>>>
  [[nodiscard]] std::set<D> ifdsResultsAt(ByConstRef<N> Stmt) const {
    return static_cast<SolverResults<N, D, L>>(*this).ifdsResultsAt(Stmt);
  }

  [[nodiscard]] std::vector<typename Table<N, D, L>::Cell>
  getAllResultEntries() const {
    return Results.cellVec();
  }

  template <typename ICFGTy>
  void dumpResults(const ICFGTy &ICF, const NodePrinterBase<n_t> &NP,
                   const DataFlowFactPrinterBase<d_t> &DP,
                   const ValuePrinter<l_t> LP,
                   llvm::raw_ostream &OS = llvm::outs()) {
    static_cast<SolverResults<N, D, L>>(*this).dumpResults(ICF, NP, DP, LP, OS);
  }

  template <typename ICFGTy, typename ProblemTy>
  void dumpResults(const ICFGTy &ICF, const ProblemTy &IDEProblem,
                   llvm::raw_ostream &OS = llvm::outs()) {
    static_cast<SolverResults<N, D, L>>(*this).dumpResults(ICF, IDEProblem, OS);
  }

private:
  // psr::Table is not const-enabled, so we have to give out mutable references
  mutable Table<N, D, L> Results;
  D ZV;
};

} // namespace psr

#endif
