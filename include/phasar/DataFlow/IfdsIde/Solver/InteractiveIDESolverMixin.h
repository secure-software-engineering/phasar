/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H

#include "phasar/Utils/Logger.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace psr {
template <typename Derived> class InteractiveIDESolverMixin {
public:
  /// Initialize the IDE solver for step-wise solving (iteratively calling
  /// next() or nextN()).
  /// For a more high-level API use solveUntil() or solveTimeout().
  ///
  /// \returns True, iff it is valid to call next() or nextN() afterwards.
  [[nodiscard]] bool initialize() { return self().doInitialize(); }

  /// Performs one tiny step towards the analysis' fixpoint. For a
  /// more high-level API use solveUntil() or solveTimeout().
  ///
  /// Requires that initialize() has been called before once and returned true
  /// and that all previous next() or nextN() calls returned true as well.
  ///
  /// \returns True, iff there are more steps to process before calling
  /// finalize()
  [[nodiscard]] bool next() { return self().doNext(); }

  /// Performs N tiny steps towards the analysis' fixpoint.
  /// For a more high-level API use solveUntil() or solveTimeout().
  ///
  /// Requires that initialize() has been called before once and returned true
  /// and that all previous next() or nextN() calls returned true as well.
  ///
  /// \returns True, iff there are more steps to process before calling
  /// finalize()
  [[nodiscard]] bool nextN(size_t MaxNumIterations) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "[nextN]: Next " << MaxNumIterations << " Iterations");

    for (size_t I = 0, End = MaxNumIterations; I != End; ++I) {
      if (!next()) {
        PHASAR_LOG_LEVEL(DEBUG, "[nextN]: > done after " << I << " iterations");
        return false;
      }
    }
    PHASAR_LOG_LEVEL(DEBUG, "[nextN]: > has next");
    return true;
  }

  /// Computes the final analysis results after the analysis has reached its
  /// fixpoint, i.e. either initialize() returned false or the last next() or
  /// nextN() call returned false.
  ///
  /// Returns a view into the computed analysis results
  decltype(auto) finalize() & { return self().doFinalize(); }
  /// Computes the final analysis results after the analysis has reached its
  /// fixpoint, i.e. either initialize() returned false or the last next() or
  /// nextN() call returned false.
  ///
  /// Returns a the computed analysis results
  decltype(auto) finalize() && { return std::move(self()).doFinalize(); }

  /// Solves the analysis problen and periodically checks with Frequency whether
  /// CancellationRequested evaluates to true.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  template <typename CancellationRequest,
            typename = std::enable_if_t<
                std::is_invocable_r_v<bool, CancellationRequest> ||
                std::is_invocable_r_v<bool, CancellationRequest,
                                      std::chrono::steady_clock::time_point>>>
  auto
  solveUntil(CancellationRequest CancellationRequested,
             std::chrono::milliseconds Frequency = std::chrono::seconds{1}) & {
    return solveUntilImpl(self(), std::move(CancellationRequested), Frequency);
  }

  /// Solves the analysis problen and periodically checks with Frequency whether
  /// CancellationRequested evaluates to true.
  ///
  /// \returns An std::optional holding the analysis results or std::nullopt if
  /// the analysis was cancelled.
  template <typename CancellationRequest,
            typename = std::enable_if_t<
                std::is_invocable_r_v<bool, CancellationRequest> ||
                std::is_invocable_r_v<bool, CancellationRequest,
                                      std::chrono::steady_clock::time_point>>>
  auto
  solveUntil(CancellationRequest CancellationRequested,
             std::chrono::milliseconds Frequency = std::chrono::seconds{1}) && {
    return solveUntilImpl(std::move(self()), std::move(CancellationRequested),
                          Frequency);
  }

  /// Solves the analysis problen and periodically checks with Frequency whether
  /// the Timeout has been exceeded.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  auto solveTimeout(std::chrono::milliseconds Timeout,
                    std::chrono::milliseconds Frequency) & {
    auto CancellatioNRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };
    return solveUntilImpl(self(), CancellatioNRequested, Frequency);
  }

  /// Solves the analysis problen and periodically checks with Frequency whether
  /// the Timeout has been exceeded.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  auto solveTimeout(std::chrono::milliseconds Timeout,
                    std::chrono::milliseconds Frequency) && {
    auto CancellatioNRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };
    return solveUntilImpl(std::move(self()), CancellatioNRequested, Frequency);
  }

private:
  [[nodiscard]] Derived &self() &noexcept {
    static_assert(std::is_base_of_v<InteractiveIDESolverMixin, Derived>,
                  "Invalid CRTP instantiation");
    return static_cast<Derived &>(*this);
  }

  template <typename SelfTy, typename CancellationRequest>
  static auto solveUntilImpl(SelfTy &&Self,
                             CancellationRequest CancellationRequested,
                             std::chrono::milliseconds Frequency) {
    using RetTy = std::optional<
        std::decay_t<decltype(std::forward<SelfTy>(Self).finalize())>>;

    size_t NumIterations = Frequency.count() * 500;
    auto IsCancellationRequested =
        [&CancellationRequested](
            std::chrono::steady_clock::time_point TimeStamp) {
          if constexpr (std::is_invocable_r_v<
                            bool, CancellationRequest,
                            std::chrono::steady_clock::time_point>) {
            return std::invoke(CancellationRequested, TimeStamp);
          } else {
            return std::invoke(CancellationRequested);
          }
        };

    if (Self.initialize()) {
      auto Start = std::chrono::steady_clock::now();

      if (IsCancellationRequested(Start)) {
        return RetTy();
      }

      while (Self.nextN(NumIterations)) {
        auto End = std::chrono::steady_clock::now();
        using milliseconds_d = std::chrono::duration<double, std::milli>;

        auto DeltaTime =
            std::chrono::duration_cast<milliseconds_d>(End - Start);
        Start = End;

        if (IsCancellationRequested(End)) {
          return RetTy();
        }

        // Adjust NumIterations
        auto IterationsPerMilli = double(NumIterations) / DeltaTime.count();
        auto NewNumIterations =
            size_t(IterationsPerMilli * double(Frequency.count()));
        NumIterations = (NumIterations + 2 * NewNumIterations) / 3;
      }
    }

    auto End = std::chrono::steady_clock::now();
    if (IsCancellationRequested(End)) {
      return RetTy();
    }
    return RetTy(std::forward<SelfTy>(Self).finalize());
  }
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H
