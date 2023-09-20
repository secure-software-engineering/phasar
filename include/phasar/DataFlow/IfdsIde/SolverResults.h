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

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVERRESULTS_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVERRESULTS_H

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

namespace detail {
template <typename Derived, typename N, typename D, typename L>
class SolverResultsBase {
public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  [[nodiscard]] ByConstRef<l_t> resultAt(ByConstRef<n_t> Stmt,
                                         ByConstRef<d_t> Node) const {
    return self().Results.get(Stmt, Node);
  }

  [[nodiscard]] std::unordered_map<d_t, l_t> resultsAt(ByConstRef<n_t> Stmt,
                                                       bool StripZero) const {
    std::unordered_map<d_t, l_t> Result = self().Results.row(Stmt);
    if (StripZero) {
      Result.erase(self().ZV);
    }
    return Result;
  }

  [[nodiscard]] const std::unordered_map<d_t, l_t> &
  resultsAt(ByConstRef<n_t> Stmt) const {
    return self().Results.row(Stmt);
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  template <typename ValueDomain = l_t,
            typename = typename std::enable_if_t<
                std::is_same_v<ValueDomain, BinaryDomain>>>
  [[nodiscard]] std::set<d_t> ifdsResultsAt(ByConstRef<n_t> Stmt) const {
    std::set<D> KeySet;
    const auto &ResultMap = self().Results.row(Stmt);
    for (const auto &[FlowFact, Val] : ResultMap) {
      KeySet.insert(FlowFact);
    }
    return KeySet;
  }

  /// Returns the data-flow results at the given statement while respecting
  /// LLVM's SSA semantics.
  ///
  /// An example: when a value is loaded and the location loaded from, here
  /// variable 'i', is a data-flow fact that holds, then the loaded value '%0'
  /// will usually be generated and also holds. However, due to the underlying
  /// theory (and respective implementation) this load instruction causes the
  /// loaded value to be generated and thus, it will be valid only AFTER the
  /// load instruction, i.e., at the successor instruction.
  ///
  ///   %0 = load i32, i32* %i, align 4
  ///
  /// This result accessor function returns the results at the successor
  /// instruction(s) reflecting that the expression on the left-hand side holds
  /// if the expression on the right-hand side holds.
  template <typename NTy = n_t>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::decay_t<std::remove_pointer_t<NTy>>,
                     llvm::Instruction>,
      std::unordered_map<d_t, l_t>>
  resultsAtInLLVMSSA(ByConstRef<n_t> Stmt, bool AllowOverapproximation = false,
                     bool StripZero = false) const;

  /// Returns the L-type result at the given statement for the given data-flow
  /// fact while respecting LLVM's SSA semantics.
  ///
  /// An example: when a value is loaded and the location loaded from, here
  /// variable 'i', is a data-flow fact that holds, then the loaded value '%0'
  /// will usually be generated and also holds. However, due to the underlying
  /// theory (and respective implementation) this load instruction causes the
  /// loaded value to be generated and thus, it will be valid only AFTER the
  /// load instruction, i.e., at the successor instruction.
  ///
  ///   %0 = load i32, i32* %i, align 4
  ///
  /// This result accessor function returns the results at the successor
  /// instruction(s) reflecting that the expression on the left-hand side holds
  /// if the expression on the right-hand side holds.
  template <typename NTy = n_t>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::decay_t<std::remove_pointer_t<NTy>>,
                     llvm::Instruction>,
      l_t>
  resultAtInLLVMSSA(ByConstRef<n_t> Stmt, d_t Value,
                    bool AllowOverapproximation = false) const;

  [[nodiscard]] std::vector<typename Table<n_t, d_t, l_t>::Cell>
  getAllResultEntries() const {
    return self().Results.cellVec();
  }

  template <typename ICFGTy>
  void dumpResults(const ICFGTy &ICF,
                   llvm::raw_ostream &OS = llvm::outs()) const {
    using f_t = typename ICFGTy::f_t;

    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", Full);
    OS << "\n***************************************************************\n"
       << "*                  Raw IDESolver results                      *\n"
       << "***************************************************************\n";
    auto Cells = self().Results.cellVec();
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
          std::string NString = NToString(Curr);
          std::string Line(NString.size(), '-');
          OS << "\n\nN: " << NString << "\n---" << Line << '\n';
        }
        OS << "\tD: " << DToString(Cells[I].getColumnKey())
           << " | V: " << LToString(Cells[I].getValue()) << '\n';
      }
    }
    OS << '\n';
    STOP_TIMER("DFA IDE Result Dumping", Full);
  }

private:
  [[nodiscard]] const Derived &self() const noexcept {
    static_assert(std::is_base_of_v<SolverResultsBase, Derived>);
    return static_cast<const Derived &>(*this);
  }
};
} // namespace detail

template <typename N, typename D, typename L>
class SolverResults
    : public detail::SolverResultsBase<SolverResults<N, D, L>, N, D, L> {
  using base_t = detail::SolverResultsBase<SolverResults<N, D, L>, N, D, L>;
  friend base_t;

public:
  using typename base_t::d_t;
  using typename base_t::l_t;
  using typename base_t::n_t;

  SolverResults(const Table<n_t, d_t, l_t> &ResTab, ByConstRef<d_t> ZV) noexcept
      : Results(ResTab), ZV(ZV) {}
  SolverResults(Table<n_t, d_t, l_t> &&ResTab, ByConstRef<d_t> ZV) = delete;

private:
  const Table<n_t, d_t, l_t> &Results;
  ByConstRef<D> ZV;
};

template <typename N, typename D, typename L>
class OwningSolverResults
    : public detail::SolverResultsBase<OwningSolverResults<N, D, L>, N, D, L> {
  using base_t =
      detail::SolverResultsBase<OwningSolverResults<N, D, L>, N, D, L>;
  friend base_t;

public:
  using typename base_t::d_t;
  using typename base_t::l_t;
  using typename base_t::n_t;

  OwningSolverResults(Table<N, D, L> ResTab,
                      D ZV) noexcept(std::is_nothrow_move_constructible_v<D>)
      : Results(std::move(ResTab)), ZV(std::move(ZV)) {}

  [[nodiscard]] SolverResults<N, D, L> get() const &noexcept {
    return {Results, ZV};
  }
  SolverResults<N, D, L> get() && = delete;

  [[nodiscard]] operator SolverResults<N, D, L>() const &noexcept {
    return get();
  }
  operator SolverResults<N, D, L>() && = delete;

private:
  Table<N, D, L> Results;
  D ZV;
};

} // namespace psr

#endif
