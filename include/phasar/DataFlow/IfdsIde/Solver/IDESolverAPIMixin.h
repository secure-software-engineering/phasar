/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDESOLVERAPIMIXIN_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDESOLVERAPIMIXIN_H

#include "phasar/Utils/Logger.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace psr {
template <typename Derived> class IDESolverAPIMixin {
public:
  /// Initialize the IDE solver for step-wise solving (iteratively calling
  /// next() or nextN()).
  /// For a more high-level API use solveUntil() or solveTimeout().
  constexpr void initialize() { self().doInitialize(); }

  /// Performs one tiny step towards the analysis' fixpoint. For a
  /// more high-level API use solveUntil() or solveTimeout().
  ///
  /// Requires that initialize() has been called before once and that all
  /// previous next() or nextN() calls returned true as well.
  ///
  /// \returns True, iff there are more steps to process before calling
  /// finalize()
  [[nodiscard]] constexpr bool next() { return self().doNext(); }

  /// Performs N tiny steps towards the analysis' fixpoint.
  /// For a more high-level API use solveUntil() or solveTimeout().
  ///
  /// Requires that initialize() has been called before once and that all
  /// previous next() or nextN() calls returned true as well.
  ///
  /// \returns True, iff there are more steps to process before calling
  /// finalize()
  [[nodiscard]] constexpr bool nextN(size_t MaxNumIterations) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "[nextN]: Next " << MaxNumIterations << " Iterations");

    for (size_t I = 0; I != MaxNumIterations; ++I) {
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
  /// \returns A view into the computed analysis results
  constexpr decltype(auto) finalize() & { return self().doFinalize(); }
  /// Computes the final analysis results after the analysis has reached its
  /// fixpoint, i.e. either initialize() returned false or the last next() or
  /// nextN() call returned false.
  ///
  /// \returns The computed analysis results
  constexpr decltype(auto) finalize() && {
    return std::move(self()).doFinalize();
  }

  /// Runs the solver on the configured problem. This can take some time and
  /// cannot be interrupted. If you need the ability to interrupt the solving
  /// process consider using solveUntil() or solveTimeout().
  ///
  /// \returns A view into the computed analysis results
  constexpr decltype(auto) solve() & {
    solveImpl();
    return finalize();
  }

  /// Runs the solver on the configured problem. This can take some time and
  /// cannot be interrupted. If you need the ability to interrupt the solving
  /// process consider using solveUntil() or solveTimeout().
  ///
  /// \returns The computed analysis results
  constexpr decltype(auto) solve() && {
    solveImpl();
    return std::move(*this).finalize();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. This can take some time and cannot be
  /// interrupted. If you need the ability to interrupt the solving process
  /// consider using solveUntil() or solveTimeout().
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// \returns A view into the computed analysis results
  constexpr decltype(auto) continueSolving() & {
    continueImpl();
    return finalize();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. This can take some time and cannot be
  /// interrupted. If you need the ability to interrupt the solving process
  /// consider using solveUntil() or solveTimeout().
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// \returns The computed analysis results
  constexpr decltype(auto) continueSolving() && {
    continueImpl();
    return std::move(*this).finalize();
  }

  /// Solves the analysis problem and periodically checks every Interval whether
  /// CancellationRequested evaluates to true.
  ///
  /// Note: Shortening the cancellation-check interval will make the
  /// cancellation-time more precise (by having more frequent calls to
  /// CancellationRequested) but also have negative impact on the solver's
  /// performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  template <typename CancellationRequest>
    requires(std::is_invocable_r_v<bool, CancellationRequest> ||
             std::is_invocable_r_v<bool, CancellationRequest,
                                   std::chrono::steady_clock::time_point>)
  constexpr auto
  solveUntil(CancellationRequest CancellationRequested,
             std::chrono::milliseconds Interval = std::chrono::seconds{1}) & {
    using RetTy = std::optional<std::decay_t<decltype(finalize())>>;
    return [&]() -> RetTy {
      if (solveUntilImpl(std::move(CancellationRequested), Interval)) {
        return finalize();
      }
      return std::nullopt;
    }();
  }

  /// Solves the analysis problem and periodically checks every Interval whether
  /// CancellationRequested evaluates to true.
  ///
  /// Note: Shortening the cancellation-check interval will make the
  /// cancellation-time more precise (by having more frequent calls to
  /// CancellationRequested) but also have negative impact on the solver's
  /// performance.
  ///
  /// \returns An std::optional holding the analysis results or std::nullopt if
  /// the analysis was cancelled.
  template <typename CancellationRequest>
    requires(std::is_invocable_r_v<bool, CancellationRequest> ||
             std::is_invocable_r_v<bool, CancellationRequest,
                                   std::chrono::steady_clock::time_point>)
  constexpr auto
  solveUntil(CancellationRequest CancellationRequested,
             std::chrono::milliseconds Interval = std::chrono::seconds{1}) && {
    using RetTy =
        std::optional<std::decay_t<decltype(std::move(*this).finalize())>>;
    return [&]() -> RetTy {
      if (solveUntilImpl(std::move(CancellationRequested), Interval)) {
        return std::move(*this).finalize();
      }
      return std::nullopt;
    }();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks every Interval whether
  /// CancellationRequested evaluates to true.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// Note: Shortening the cancellation-check interval will make the
  /// cancellation-time more precise (by having more frequent calls to
  /// CancellationRequested) but also have negative impact on the solver's
  /// performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  template <typename CancellationRequest>
    requires(std::is_invocable_r_v<bool, CancellationRequest> ||
             std::is_invocable_r_v<bool, CancellationRequest,
                                   std::chrono::steady_clock::time_point>)
  constexpr auto continueUntil(
      CancellationRequest CancellationRequested,
      std::chrono::milliseconds Interval = std::chrono::seconds{1}) & {
    using RetTy = std::optional<std::decay_t<decltype(finalize())>>;
    return [&]() -> RetTy {
      if (continueUntilImpl(std::move(CancellationRequested), Interval)) {
        return finalize();
      }
      return std::nullopt;
    }();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks every Interval whether
  /// CancellationRequested evaluates to true.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// Note: Shortening the cancellation-check interval will make the
  /// cancellation-time more precise (by having more frequent calls to
  /// CancellationRequested) but also have negative impact on the solver's
  /// performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  template <typename CancellationRequest>
    requires(std::is_invocable_r_v<bool, CancellationRequest> ||
             std::is_invocable_r_v<bool, CancellationRequest,
                                   std::chrono::steady_clock::time_point>)
  constexpr auto continueUntil(
      CancellationRequest CancellationRequested,
      std::chrono::milliseconds Interval = std::chrono::seconds{1}) && {
    using RetTy =
        std::optional<std::decay_t<decltype(std::move(*this).finalize())>>;
    return [&]() -> RetTy {
      if (continueUntilImpl(std::move(CancellationRequested), Interval)) {
        return std::move(*this).finalize();
      }
      return std::nullopt;
    }();
  }

  /// Solves the analysis problem and periodically checks every Interval whether
  /// the Timeout has been exceeded.
  ///
  /// Note: Shortening the timeout-check interval will make the timeout more
  /// precise but also have negative impact on the solver's performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto solveWithTimeout(std::chrono::milliseconds Timeout,
                                  std::chrono::milliseconds Interval) & {
    auto CancellationRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };

    return solveUntil(CancellationRequested, Interval);
  }

  /// Solves the analysis problem and periodically checks every Interval whether
  /// the Timeout has been exceeded.
  ///
  /// Note: Shortening the timeout-check interval will make the timeout more
  /// precise but also have negative impact on the solver's performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto solveWithTimeout(std::chrono::milliseconds Timeout,
                                  std::chrono::milliseconds Interval) && {
    auto CancellatioNRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };
    return std::move(*this).solveUntil(CancellatioNRequested, Interval);
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks every Interval whether
  /// the Timeout has been exceeded.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// Note: Shortening the timeout-check interval will make the timeout more
  /// precise but also have negative impact on the solver's performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto continueWithTimeout(std::chrono::milliseconds Timeout,
                                     std::chrono::milliseconds Interval) & {
    auto CancellationRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };

    return continueUntil(CancellationRequested, Interval);
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks every Interval whether
  /// the Timeout has been exceeded.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// Note: Shortening the timeout-check interval will make the timeout more
  /// precise but also have negative impact on the solver's performance.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto continueWithTimeout(std::chrono::milliseconds Timeout,
                                     std::chrono::milliseconds Interval) && {
    auto CancellationRequested =
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        };

    return std::move(*this).continueUntil(CancellationRequested, Interval);
  }

  // -- Async cancellation

  /// Solves the analysis problem and periodically checks whether
  /// IsCancelled is true.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto solveWithAsyncCancellation(std::atomic_bool &IsCancelled) & {
    using RetTy = std::optional<std::decay_t<decltype(finalize())>>;
    return [&]() -> RetTy {
      if (solveWithAsyncCancellationImpl(IsCancelled)) {
        return finalize();
      }
      return std::nullopt;
    }();
  }

  /// Solves the analysis problem and periodically checks whether
  /// IsCancelled is true.
  ///
  /// \returns An std::optional holding the analysis results or std::nullopt if
  /// the analysis was cancelled.
  constexpr auto solveWithAsyncCancellation(std::atomic_bool &IsCancelled) && {
    using RetTy =
        std::optional<std::decay_t<decltype(std::move(*this).finalize())>>;
    return [&]() -> RetTy {
      if (solveWithAsyncCancellationImpl(IsCancelled)) {
        return std::move(*this).finalize();
      }
      return std::nullopt;
    }();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks whether
  /// IsCancelled is true.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto
  continueWithAsyncCancellation(std::atomic_bool &IsCancelled) & {
    using RetTy = std::optional<std::decay_t<decltype(finalize())>>;
    return [&]() -> RetTy {
      if (continueWithAsyncCancellationImpl(IsCancelled)) {
        return finalize();
      }
      return std::nullopt;
    }();
  }

  /// Continues running the solver on the configured problem after it got
  /// interrupted in a previous run. Periodically checks whether
  /// IsCancelled is true.
  ///
  /// \remark Please make sure to *only* call this function on an IDESolver
  /// where the solving process is interrupted, i.e. one of the interruptable
  /// solving methods returned std::nullopt. It is *invalid* to call this
  /// function on an IDESolver that has not yet started solving or has already
  /// flinalized solving.
  ///
  /// \returns An std::optional holding a view into the analysis results or
  /// std::nullopt if the analysis was cancelled.
  constexpr auto
  continueWithAsyncCancellation(std::atomic_bool &IsCancelled) && {
    using RetTy =
        std::optional<std::decay_t<decltype(std::move(*this).finalize())>>;
    return [&]() -> RetTy {
      if (continueWithAsyncCancellationImpl(IsCancelled)) {
        return std::move(*this).finalize();
      }
      return std::nullopt;
    }();
  }

private:
  friend Derived;
  constexpr IDESolverAPIMixin() noexcept = default;

  [[nodiscard]] constexpr Derived &self() & noexcept {
    static_assert(std::is_base_of_v<IDESolverAPIMixin, Derived>,
                  "Invalid CRTP instantiation");
    return static_cast<Derived &>(*this);
  }

  constexpr void continueImpl() {
    while (next()) {
      // no interrupt in normal solving process
    }
  }

  constexpr void solveImpl() {
    initialize();
    continueImpl();
  }

  template <typename CancellationRequest>
  [[nodiscard]] constexpr bool
  continueUntilImpl(CancellationRequest CancellationRequested,
                    std::chrono::milliseconds Interval) {
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

    // Some initial number of propagations to get an idea, how long a
    // propagation takes. This may be adjusted in the future
    size_t NumIterations = Interval.count() * 500;

    auto Start = std::chrono::steady_clock::now();

    while (nextN(NumIterations)) {
      auto End = std::chrono::steady_clock::now();
      using milliseconds_d = std::chrono::duration<double, std::milli>;

      auto DeltaTime = std::chrono::duration_cast<milliseconds_d>(End - Start);
      Start = End;

      if (IsCancellationRequested(End)) {
        return false;
      }

      // Adjust NumIterations
      auto IterationsPerMilli = double(NumIterations) / DeltaTime.count();
      auto NewNumIterations =
          size_t(IterationsPerMilli * double(Interval.count()));
      NumIterations = (NumIterations + 2 * NewNumIterations) / 3;
    }
    auto End = std::chrono::steady_clock::now();
    return !IsCancellationRequested(End);
  }

  template <typename CancellationRequest>
  [[nodiscard]] constexpr bool
  solveUntilImpl(CancellationRequest CancellationRequested,
                 std::chrono::milliseconds Interval) {
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

    initialize();
    auto TimeStamp = std::chrono::steady_clock::now();
    if (IsCancellationRequested(TimeStamp)) {
      return false;
    }

    return continueUntilImpl(std::move(CancellationRequested), Interval);
  }

  [[nodiscard]] constexpr bool
  continueWithAsyncCancellationImpl(std::atomic_bool &IsCancelled) {
    while (next()) {
      if (IsCancelled.load()) {
        return false;
      }
    }
    return !IsCancelled.load();
  }

  [[nodiscard]] constexpr bool
  solveWithAsyncCancellationImpl(std::atomic_bool &IsCancelled) {
    initialize();
    if (IsCancelled.load()) {
      return false;
    }

    return continueWithAsyncCancellationImpl(IsCancelled);
  }
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDESOLVERAPIMIXIN_H
