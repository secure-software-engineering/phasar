/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_CHRONO_UTILS_H
#define PHASAR_PHASARLLVM_UTILS_CHRONO_UTILS_H

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <chrono>

namespace psr {

struct hms { // NOLINT
  std::chrono::hours Hours{};
  std::chrono::minutes Minutes{};
  std::chrono::seconds Seconds{};
  std::chrono::microseconds Micros{};

  hms() noexcept = default;
  hms(std::chrono::nanoseconds NS) noexcept {
    using namespace std::chrono;

    Hours = duration_cast<hours>(NS);
    NS -= Hours;
    Minutes = duration_cast<minutes>(NS);
    NS -= Minutes;
    Seconds = duration_cast<seconds>(NS);
    NS -= Seconds;
    Micros = duration_cast<microseconds>(NS);
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const hms &HMS);

  [[nodiscard]] std::string str() const {
    std::string Ret;
    llvm::raw_string_ostream OS(Ret);
    OS << *this;
    return Ret;
  }
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_CHRONO_UTILS_H
