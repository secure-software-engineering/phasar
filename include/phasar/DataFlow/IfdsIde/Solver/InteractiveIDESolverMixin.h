/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H

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
  [[nodiscard]] bool initialize() { return self().doInitialize(); }
  [[nodiscard]] bool next() { return self().doNext(); }
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
  decltype(auto) finalize() & { return self().doFinalize(); }
  decltype(auto) finalize() && { return std::move(self()).doFinalize(); }

  /// Solves the analysis problen and periodically checks with Frequency whether
  /// CancellationRequested evaluates to true.
  ///
  /// \returns True, iff the solving process was cancelled, else false.
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
  /// \returns True, iff the solving process was cancelled, else false.
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
  /// \returns True, iff the solving process was cancelled, else false.
  bool solveTimeout(std::chrono::milliseconds Timeout,
                    std::chrono::milliseconds Frequency) {
    return solveUntil(
        [Timeout, Start = std::chrono::steady_clock::now()](
            std::chrono::steady_clock::time_point TimeStamp) {
          return TimeStamp - Start >= Timeout;
        },
        Frequency);
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
    if (!Self.initialize()) {
      return RetTy(std::forward<SelfTy>(Self).finalize());
    }

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

    auto Start = std::chrono::steady_clock::now();

    if (IsCancellationRequested(Start)) {
      return RetTy();
    }

    while (true) {
      if (!Self.nextN(NumIterations)) {
        return RetTy(std::forward<SelfTy>(Self).finalize());
      }

      auto End = std::chrono::steady_clock::now();
      using milliseconds_d = std::chrono::duration<double, std::milli>;

      auto DeltaTime = std::chrono::duration_cast<milliseconds_d>(End - Start);
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

    llvm_unreachable(
        "We do not break out the above loop except for explicit returns");
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_INTERACTIVEIDESOLVERMIXIN_H
