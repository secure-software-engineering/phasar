/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_AVERAGE_H
#define PHASAR_UTILS_AVERAGE_H

#include <cstddef>

namespace psr {

struct Sampler {
  size_t Count{};
  double Curr{};

  constexpr void addSample(size_t Sample) noexcept {
    Curr += (double(Sample) - Curr) / double(++Count);
  }

  [[nodiscard]] constexpr double getAverage() const noexcept { return Curr; }
  [[nodiscard]] constexpr size_t getNumSamples() const noexcept {
    return Count;
  }
};

} // namespace psr

#endif // PHASAR_UTILS_AVERAGE_H
