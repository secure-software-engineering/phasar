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

#include "llvm/ADT/FunctionExtras.h"

#include <chrono>

namespace psr {
class Timer {
public:
  Timer(llvm::unique_function<void(std::chrono::nanoseconds)>
            WithElapsed) noexcept
      : WithElapsed(std::move(WithElapsed)),
        Start(std::chrono::steady_clock::now()) {}

  Timer(Timer &&) noexcept = default;
  Timer &operator=(Timer &&) noexcept = default;
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;

  ~Timer() {
    if (WithElapsed) {
      auto End = std::chrono::steady_clock::now();
      WithElapsed(End - Start);
    }
  }

private:
  llvm::unique_function<void(std::chrono::nanoseconds)> WithElapsed;
  std::chrono::steady_clock::time_point Start;
};
} // namespace psr

#endif // PHASAR_UTILS_TIMER_H
