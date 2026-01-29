#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H

#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverResults.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/STLExtras.h"

#include <cassert>
#include <cstddef>
#include <set>
#include <unordered_map>
#include <utility>

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
      mutable std::optional<std::pair<d_t, l_t>> Cache{};

      const std::pair<d_t, l_t> &
      operator()(ByConstRef<typename row_map_t::value_type> Entry) const {
        Cache = std::make_pair(Results->FactCompressor[Entry.first],
                               Results->ValCompressor[Entry.second]);
        return *Cache;
      }
    };

  public:
    using iterator = llvm::mapped_iterator<typename row_map_t::const_iterator,
                                           Transformator>;
    using const_iterator = iterator;
    using difference_type = ptrdiff_t;

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

private:
  [[nodiscard]] constexpr const auto &resultsImpl() const noexcept {
    assert(Results != nullptr);
    return *Results;
  }

  std::unique_ptr<const detail::IterativeIDESolverResults<N, D, L>> Results{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDBASEDSOLVERRESULTS_H
