#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H

#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverResults.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstddef>
#include <set>
#include <unordered_map>
#include <utility>

namespace llvm {
class Instruction;
class Value;
} // namespace llvm

namespace psr {

namespace detail {
template <typename Derived, typename N, typename D, typename L>
class IdBasedSolverResultsBase {
  using row_map_t =
      typename detail::IterativeIDESolverResults<N, D, L>::ValTab_value_type;

public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  class RowView {
    struct Transformator {
      const detail::IterativeIDESolverResults<n_t, d_t, l_t> *Results{};

      // --v Needed for llvm::mapped_iterator
      // NOLINTNEXTLINE(readability-const-return-type)
      const std::pair<ByConstRef<d_t>, ByConstRef<l_t>>
      operator()(ByConstRef<typename row_map_t::value_type> Entry) const {
        return {Results->FactCompressor[Entry.first],
                Results->ValCompressor[Entry.second]};
      }
    };

  public:
    using iterator = llvm::mapped_iterator<typename row_map_t::const_iterator,
                                           Transformator>;
    using const_iterator = iterator;
    using difference_type = ptrdiff_t;

    using key_type = d_t;
    using mapped_type = l_t;
    using value_type = std::pair<d_t, l_t>;

    explicit RowView(
        const detail::IterativeIDESolverResults<n_t, d_t, l_t> *Results,
        const row_map_t *Row) noexcept
        : Results(Results), Row(Row) {
      assert(Results != nullptr);
      assert(Row != nullptr);
    }

    [[nodiscard]] const_iterator begin() const noexcept {
      return llvm::map_iterator(Row->cells().begin(), Transformator{Results});
    }

    [[nodiscard]] const_iterator end() const noexcept {
      return llvm::map_iterator(Row->cells().end(), Transformator{Results});
    }

    [[nodiscard]] const_iterator find(ByConstRef<d_t> Fact) const {
      auto FactId = Results->FactCompressor.getOrNull(Fact);
      if (!FactId) {
        return end();
      }
      return llvm::map_iterator(Row->find(*FactId), Transformator{Results});
    }

    [[nodiscard]] bool count(ByConstRef<d_t> Fact) const {
      auto FactId = Results->FactCompressor.getOrNull(Fact);
      if (!FactId) {
        return false;
      }
      return Row->contains(*FactId);
    }

    [[nodiscard]] size_t size() const noexcept { return Row->size(); }
    [[nodiscard]] bool empty() const noexcept { return Row->empty(); }

  private:
    const detail::IterativeIDESolverResults<n_t, d_t, l_t> *Results{};
    const row_map_t *Row{};
  };

  [[nodiscard]] l_t resultAt(ByConstRef<n_t> Stmt, ByConstRef<d_t> Node) const {
    auto NodeId = results().NodeCompressor.getOrNull(Stmt);
    auto FactId = results().FactCompressor.getOrNull(Node);

    if (!NodeId || !FactId) {
      return l_t{};
    }

    const auto &Entry = results().ValTab[size_t(*NodeId)];
    auto RetIt = Entry.find(*FactId);
    if (RetIt == Entry.cells().end()) {
      return l_t{};
    }

    return results().ValCompressor[RetIt->second];
  }

  [[nodiscard]] std::unordered_map<d_t, l_t>
  resultsAt(ByConstRef<n_t> Stmt, bool StripZero = false) const {
    auto NodeId = results().NodeCompressor.getOrNull(Stmt);
    if (!NodeId) {
      return {};
    }

    std::unordered_map<d_t, l_t> Result;
    Result.reserve(results().ValTab[size_t(*NodeId)].size());
    for (auto [Fact, Value] : results().ValTab[size_t(*NodeId)].cells()) {
      /// In the IterativeIDESolver, we have made sure that the zero flow-fact
      /// always has the Id 0
      if (StripZero && Fact == 0) {
        continue;
      }
      Result.try_emplace(results().FactCompressor[Fact],
                         results().ValCompressor[Value]);
    }

    return Result;
  }

  [[nodiscard]] std::set<d_t> ifdsResultsAt(ByConstRef<n_t> Stmt) const {
    auto NodeId = results().NodeCompressor.getOrNull(Stmt);
    if (!NodeId) {
      return {};
    }

    std::set<d_t> Result;
    for (auto [Fact, Unused] : results().ValTab[size_t(*NodeId)]) {
      Result.insert(results().FactCompressor[Fact]);
    }
    return Result;
  }

  [[nodiscard]] size_t size() const noexcept {
    assert(results().ValTab.size() >= results().NodeCompressor.size());
    return results().NodeCompressor.size();
  }

  [[nodiscard]] auto getAllResultEntries() const noexcept {
    auto Txn =
        [Results{&results()}](const auto &Entry) -> std::pair<n_t, RowView> {
      const auto &[First, Second] = Entry;
      return std::make_pair(First, RowView(Results, &Second));
    };

    return llvm::map_range(
        llvm::zip(results().NodeCompressor, results().ValTab), Txn);
  }

  [[nodiscard]] bool containsNode(ByConstRef<n_t> Stmt) const {
    return results().NodeCompressor.getOrNull(Stmt) != std::nullopt;
  }

  [[nodiscard]] RowView row(ByConstRef<n_t> Stmt) const {
    auto NodeId = results().NodeCompressor.getOrNull(Stmt);
    assert(NodeId);
    return RowView(&results(), &results().ValTab[*NodeId]);
  }

  template <typename HandlerFn>
  void foreachResultEntry(HandlerFn Handler) const {
    for (const auto &[Row, RowMap] : getAllResultEntries()) {
      for (const auto &[Col, Val] : RowMap) {
        std::invoke(Handler, std::make_tuple(Row, Col, Val));
      }
    }
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
  resultsAtInLLVMSSA(ByConstRef<n_t> Stmt,
                     bool AllowOverapproximation = false) const
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

  template <typename ICFGTy>
  void dumpResults(const ICFGTy &ICF,
                   llvm::raw_ostream &OS = llvm::outs()) const;

private:
  [[nodiscard]] constexpr const auto &results() const noexcept {
    return static_cast<const Derived *>(this)->resultsImpl();
  }
};
} // namespace detail

template <typename N, typename D, typename L>
class IdBasedSolverResults
    : public detail::IdBasedSolverResultsBase<IdBasedSolverResults<N, D, L>, N,
                                              D, L> {
  friend detail::IdBasedSolverResultsBase<IdBasedSolverResults<N, D, L>, N, D,
                                          L>;

public:
  explicit IdBasedSolverResults(
      const detail::IterativeIDESolverResults<N, D, L> *Results) noexcept
      : Results(Results) {
    assert(Results != nullptr);
  }

private:
  [[nodiscard]] constexpr const auto &resultsImpl() const noexcept {
    assert(Results != nullptr);
    return *Results;
  }

  const detail::IterativeIDESolverResults<N, D, L> *Results{};
};

template <typename N, typename D, typename L>
class OwningIdBasedSolverResults
    : public detail::IdBasedSolverResultsBase<
          OwningIdBasedSolverResults<N, D, L>, N, D, L> {
  friend detail::IdBasedSolverResultsBase<OwningIdBasedSolverResults<N, D, L>,
                                          N, D, L>;

public:
  explicit OwningIdBasedSolverResults(
      std::unique_ptr<const detail::IterativeIDESolverResults<N, D, L>>
          Results) noexcept
      : Results(std::move(Results)) {
    assert(this->Results != nullptr);
  }

  [[nodiscard]] IdBasedSolverResults<N, D, L> get() const & noexcept {
    return IdBasedSolverResults<N, D, L>{Results.get()};
  }
  IdBasedSolverResults<N, D, L> get() && = delete;

  [[nodiscard]] operator IdBasedSolverResults<N, D, L>() const & noexcept {
    return get();
  }

  operator IdBasedSolverResults<N, D, L>() && = delete;

private:
  [[nodiscard]] constexpr const auto &resultsImpl() const noexcept {
    assert(Results != nullptr);
    return *Results;
  }

  std::unique_ptr<const detail::IterativeIDESolverResults<N, D, L>> Results{};
};

// For sorting the results in dumpResults()
std::string getMetaDataID(const llvm::Value *V);

template <typename Derived, typename N, typename D, typename L>
template <typename ICFGTy>
void detail::IdBasedSolverResultsBase<Derived, N, D, L>::dumpResults(
    const ICFGTy &ICF, llvm::raw_ostream &OS) const {
  using f_t = typename ICFGTy::f_t;

  auto ResultEntries = llvm::to_vector(getAllResultEntries());

  std::ranges::sort(ResultEntries, [](const auto &Lhs, const auto &Rhs) {
    const auto &LRow = std::get<0>(Lhs);
    const auto &RRow = std::get<0>(Rhs);
    if constexpr (std::is_same_v<n_t, const llvm::Instruction *>) {
      return StringIDLess{}(getMetaDataID(LRow), getMetaDataID(RRow));
    } else {
      // If non-LLVM IR is used
      return LRow < RRow;
    }
  });

  OS << "\n***************************************************************\n"
     << "*                  Raw IterativeIDESolver results             *\n"
     << "***************************************************************\n";

  f_t PrevFn = f_t{};
  f_t CurrFn = f_t{};

  for (const auto &[Row, Column] : ResultEntries) {
    if (Column.empty()) {
      continue;
    }
    CurrFn = ICF.getFunctionOf(Row);
    if (PrevFn != CurrFn) {
      PrevFn = CurrFn;
      OS << "\n\n============ Results for function '" +
                ICF.getFunctionName(CurrFn) + "' ============\n";
    }

    std::string NString = NToString(Row);
    std::string Line(NString.size(), '-');
    OS << "\n\nN: " << NString << "\n---" << Line << '\n';

    for (const auto &[Col, Val] : Column) {
      OS << "\tD: " << DToString(Col) << " | V: " << LToString(Val) << '\n';
    }
  }
  OS << '\n';
}
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H
