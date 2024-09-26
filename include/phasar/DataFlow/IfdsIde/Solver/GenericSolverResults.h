/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_GENERICSOLVERRESULTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_GENERICSOLVERRESULTS_H

#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <new>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace psr {

/// XXX: When upgrading to C++20, create a concept checking valid SolverResults
/// types

/// A type-erased version of the main functionality of SolverResults.
/// Can be accepted by consumers that don't need deep access to the internals
/// (so, the usual ones). As we have now two kinds of solver-results
/// (SolverResults and IdBasedSolverResults), we need a common way of accessing
/// them.
template <typename N, typename D, typename L> class GenericSolverResults final {
public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  /// NOTE: Don't provide an operator= that constructs a GenericSolverResults
  /// from a concrete SolverResults object to prevent issues with object
  /// lifetime. Probably that can be mitigated with use of std::launder, but for
  /// now, we don't need this complexity

  template <typename T, typename = std::enable_if_t<!std::is_same_v<
                            GenericSolverResults, std::decay_t<T>>>>
  GenericSolverResults(const T &SR) noexcept : VT(&VtableFor<std::decay_t<T>>) {
    using type = std::decay_t<T>;

    static_assert(std::is_trivially_destructible_v<type>);
    static_assert(std::is_trivially_copyable_v<type>,
                  "SolverResults should only be a *view* into the results of "
                  "an IFDS/IDE solver.");
    static_assert(alignof(type) <= alignof(void *));
    static_assert(sizeof(type) <= sizeof(Buffer));

    static_assert(std::is_trivially_copyable_v<GenericSolverResults>);

    new (Buffer.data()) type(SR);
  }

  GenericSolverResults(const GenericSolverResults &) noexcept = default;
  GenericSolverResults &
  operator=(const GenericSolverResults &) noexcept = default;
  ~GenericSolverResults() = default;

  [[nodiscard]] l_t resultAt(ByConstRef<n_t> Stmt, ByConstRef<d_t> Node) const {
    assert(VT != nullptr);
    return VT->ResultAt(Buffer.data(), Stmt, Node);
  }

  [[nodiscard]] std::unordered_map<d_t, l_t>
  resultsAt(ByConstRef<n_t> Stmt, bool StripZero = false) const {
    assert(VT != nullptr);
    return VT->ResultsAt(Buffer.data(), Stmt, StripZero);
  }

  template <typename LL = L,
            typename = std::enable_if_t<std::is_same_v<LL, BinaryDomain>>>
  [[nodiscard]] std::set<d_t> ifdsResultsAt(ByConstRef<n_t> Stmt) const {
    assert(VT != nullptr);
    assert(VT->IfdsResultsAt != nullptr);
    return VT->IfdsResultsAt(Buffer.data(), Stmt);
  }

  [[nodiscard]] size_t size() const noexcept {
    assert(VT != nullptr);
    return VT->Size(Buffer.data());
  }

  [[nodiscard]] bool containsNode(ByConstRef<n_t> Stmt) const {
    return VT->ContainsNode(Buffer.data(), Stmt);
  }

  template <typename NN = N, typename = std::enable_if_t<
                                 std::is_same_v<NN, const llvm::Instruction *>>>
  [[nodiscard]] l_t resultAtInLLVMSSA(const llvm::Instruction *Stmt,
                                      ByConstRef<d_t> Node) const {
    if (const auto *Next = Stmt->getNextNode()) {
      return resultAt(Next, Node);
    }
    return resultAt(Stmt, Node);
  }

  template <typename NN = N, typename = std::enable_if_t<
                                 std::is_same_v<NN, const llvm::Instruction *>>>
  [[nodiscard]] std::unordered_map<d_t, l_t>
  resultsAtInLLVMSSA(const llvm::Instruction *Stmt,
                     bool StripZero = false) const {
    if (const auto *Next = Stmt->getNextNode()) {
      return resultsAt(Next, StripZero);
    }
    return resultsAt(Stmt, StripZero);
  }

  void foreachResultEntry(
      llvm::function_ref<void(std::tuple<n_t, d_t, l_t>)> Handler) const {
    assert(VT != nullptr);
    VT->ForeachResultEntry(Buffer.data(), Handler);
  }

private:
  struct VTableTy {
    l_t (*ResultAt)(const void *, ByConstRef<n_t>, ByConstRef<d_t>);
    std::unordered_map<d_t, l_t> (*ResultsAt)(const void *, ByConstRef<n_t>,
                                              bool);
    std::set<d_t> (*IfdsResultsAt)(const void *, ByConstRef<n_t>);
    size_t (*Size)(const void *) noexcept;
    bool (*ContainsNode)(const void *, ByConstRef<n_t>);
    void (*ForeachResultEntry)(
        const void *, llvm::function_ref<void(std::tuple<n_t, d_t, l_t>)>);
  };

  template <typename T>
  static constexpr VTableTy VtableFor{
      [](const void *SR, ByConstRef<n_t> Stmt, ByConstRef<d_t> Node) -> l_t {
        return static_cast<const T *>(SR)->resultAt(Stmt, Node);
      },
      [](const void *SR, ByConstRef<n_t> Stmt,
         bool StripZero) -> std::unordered_map<d_t, l_t> {
        return static_cast<const T *>(SR)->resultsAt(Stmt, StripZero);
      },
      [] {
        std::set<d_t> (*IfdsResultsAt)(const void *, ByConstRef<n_t>) = nullptr;
        if constexpr (std::is_same_v<BinaryDomain, l_t>) {
          IfdsResultsAt = [](const void *SR, ByConstRef<n_t> Stmt) {
            return static_cast<const T *>(SR)->ifdsResultsAt(Stmt);
          };
        }
        return IfdsResultsAt;
      }(),
      [](const void *SR) noexcept -> size_t {
        return static_cast<const T *>(SR)->size();
      },
      [](const void *SR, ByConstRef<n_t> Stmt) -> bool {
        return static_cast<const T *>(SR)->containsNode(Stmt);
      },
      [](const void *SR,
         llvm::function_ref<void(std::tuple<n_t, d_t, l_t>)> Handler) {
        static_cast<const T *>(SR)->foreachResultEntry(Handler);
      }};

  const VTableTy *VT{};
  alignas(alignof(void *)) std::array<std::byte, 3 * sizeof(void *)> Buffer{};
};

template <typename SR1, typename SR2>
bool ifdsEqual(const SR1 &LHS, const SR2 &RHS) {
  if (LHS.size() != RHS.size()) {
    return false;
  }

  for (const auto &[Row, ColVal] : LHS.rowMapView()) {
    if (!RHS.containsNode(Row)) {
      return false;
    }

    const auto &OtherColVal = LHS.row(Row);
    if (ColVal.size() != OtherColVal.size()) {
      return false;
    }

    for (const auto &[Col, Val] : ColVal) {
      if (!OtherColVal.count(Col)) {
        return false;
      }
    }
  }
  return true;
}

template <typename SR1, typename SR2>
bool checkSREquality(const SR1 &LHS, const SR2 &RHS) {
  bool HasError = false;
  if (LHS.size() != RHS.size()) {
    HasError = true;
    llvm::errs() << "The results sizes do not match: " << LHS.size() << " vs "
                 << RHS.size() << '\n';
  }

  auto ToString = [](const auto &Fact) {
    if constexpr (std::is_pointer_v<std::decay_t<decltype(Fact)>> &&
                  std::is_base_of_v<llvm::Value,
                                    std::decay_t<std::remove_pointer_t<
                                        std::decay_t<decltype(Fact)>>>>) {
      return llvmIRToString(Fact);
    } else {
      std::string S;
      llvm::raw_string_ostream ROS(S);
      ROS << Fact;
      ROS.flush();
      return S;
    }
  };

  for (const auto &[Row, ColVal] : LHS.rowMapView()) {
    if (!RHS.containsNode(Row)) {
      HasError = true;
      llvm::errs() << "RHS does not contain row for Inst: " << ToString(Row)
                   << '\n';
    }

    auto RHSColVal = RHS.row(Row);

    if (ColVal.size() != RHSColVal.size()) {
      HasError = true;
      llvm::errs() << "The number of facts at " << ToString(Row)
                   << " differ: " << ColVal.size() << " vs " << RHSColVal.size()
                   << '\n';
    }

    for (const auto &[Col, Val] : ColVal) {
      auto It = RHSColVal.find(Col);

      if (It == RHSColVal.end()) {
        HasError = true;
        llvm::errs() << "RHS does not contain fact " << ToString(Col)
                     << " at Inst " << ToString(Row) << '\n';
      } else if (Val != It->second) {
        HasError = true;
        llvm::errs() << "The edge values at inst " << ToString(Row)
                     << " and fact " << ToString(Col)
                     << " differ: " << ToString(Val) << " vs "
                     << ToString(It->second) << '\n';
      }
    }
  }
  return !HasError;
}
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_GENERICSOLVERRESULTS_H
