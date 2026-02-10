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
#include "phasar/Utils/TypeTraits.h"
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

  /// Returns the result that the IDE analysis computed for the fact Node right
  /// after the statement Stmt.
  ///
  /// A default-constructed l_t, if no analysis result was computed at this
  /// point.
  [[nodiscard]] ByConstRef<l_t> resultAt(ByConstRef<n_t> Stmt,
                                         ByConstRef<d_t> Node) const {
    return self().Results.get(Stmt, Node);
  }

  /// Returns the results that the IDE analysis computed right after the
  /// statement Stmt.
  ///
  /// \param Stmt The statement, where the analysis results are requested
  /// \param StripZero Whether the special zero value should be stripped from
  /// the result.
  [[nodiscard]] std::unordered_map<d_t, l_t> resultsAt(ByConstRef<n_t> Stmt,
                                                       bool StripZero) const {
    std::unordered_map<d_t, l_t> Result = self().Results.row(Stmt);
    if (StripZero) {
      Result.erase(self().ZV);
    }
    return Result;
  }

  /// Returns the results that the IDE analysis computed right after the
  /// statement Stmt.
  ///
  /// Does not strip the special zero value from the result.
  [[nodiscard]] const std::unordered_map<d_t, l_t> &
  resultsAt(ByConstRef<n_t> Stmt) const {
    return self().Results.row(Stmt);
  }

  /// The internal representation of this SolverResults object.
  [[nodiscard]] const auto &rowMapView() const {
    return self().Results.rowMapView();
  }

  /// Whether the analysis has computed any results for the statement Stmt.
  [[nodiscard]] bool containsNode(ByConstRef<N> Stmt) const {
    return self().Results.containsRow(Stmt);
  }

  /// Similar to resultsAt(ByConstRef<N>).
  [[nodiscard]] const auto &row(ByConstRef<N> Stmt) const {
    return self().Results.row(Stmt);
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  [[nodiscard]] std::set<d_t> ifdsResultsAt(ByConstRef<n_t> Stmt) const
    requires std::is_same_v<l_t, BinaryDomain>
  {
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
  [[nodiscard]] std::unordered_map<d_t, l_t>
  resultsAtInLLVMSSA(ByConstRef<n_t> Stmt, bool AllowOverapproximation = false,
                     bool StripZero = false) const
    requires same_as_decay<std::remove_pointer_t<n_t>, llvm::Instruction>;

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
  [[nodiscard]] l_t resultAtInLLVMSSA(ByConstRef<n_t> Stmt, d_t Value,
                                      bool AllowOverapproximation = false) const
    requires same_as_decay<std::remove_pointer_t<n_t>, llvm::Instruction>;

  [[nodiscard]] std::vector<typename Table<n_t, d_t, l_t>::Cell>
  getAllResultEntries() const {
    return self().Results.cellVec();
  }

  [[nodiscard]] size_t size() const noexcept { return self().Results.size(); }

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

  template <typename HandlerFn>
  void foreachResultEntry(HandlerFn Handler) const {
    for (const auto &[Row, RowMap] : rowMapView()) {
      for (const auto &[Col, Val] : RowMap) {
        std::invoke(Handler, std::make_tuple(Row, Col, Val));
      }
    }
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

  [[nodiscard]] SolverResults<N, D, L> get() const & noexcept {
    return {Results, ZV};
  }
  SolverResults<N, D, L> get() && = delete;

  [[nodiscard]] operator SolverResults<N, D, L>() const & noexcept {
    return get();
  }

  operator SolverResults<N, D, L>() && = delete;

private:
  Table<N, D, L> Results;
  D ZV;
};

} // namespace psr

#endif
