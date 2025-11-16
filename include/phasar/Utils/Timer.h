/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_TIMER_H
#define PHASAR_UTILS_TIMER_H

#include "phasar/Utils/ChronoUtils.h"

#include "llvm/ADT/FunctionExtras.h"

#include <chrono>

namespace psr {

class SimpleTimer {
public:
  SimpleTimer() noexcept : Start(std::chrono::steady_clock::now()) {}

  [[nodiscard]] hms elapsed() const noexcept {
    auto End = std::chrono::steady_clock::now();
    return {End - Start};
  }
  [[nodiscard]] std::chrono::nanoseconds elapsedNanos() const noexcept {
    auto End = std::chrono::steady_clock::now();
    return End - Start;
  }

  void restart() noexcept { Start = std::chrono::steady_clock::now(); }

private:
  std::chrono::steady_clock::time_point Start;
};

class Timer : public SimpleTimer {
public:
  Timer(llvm::unique_function<void(std::chrono::nanoseconds)>
            WithElapsed) noexcept
      : WithElapsed(std::move(WithElapsed)) {}

  Timer(Timer &&) noexcept = default;
  Timer &operator=(Timer &&) noexcept = default;
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;

  ~Timer() {
    if (WithElapsed) {
      WithElapsed(elapsedNanos());
    }
  }

private:
  llvm::unique_function<void(std::chrono::nanoseconds)> WithElapsed;
};
} // namespace psr

#endif // PHASAR_UTILS_TIMER_H
