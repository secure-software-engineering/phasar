/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_ANALYSISPROPERTIES_H
#define PHASAR_UTILS_ANALYSISPROPERTIES_H

#include "llvm/Support/raw_ostream.h"

#include <string>
#include <type_traits>

namespace psr {
enum class AnalysisProperties {
  None = 0,
  FlowSensitive = (1 << 0),
  ContextSensitive = (1 << 1),
  FieldSensitive = (1 << 2),
};

std::string to_string(AnalysisProperties Prop);

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     AnalysisProperties Prop) {
  return OS << to_string(Prop);
}

template <typename Derived> class AnalysisPropertiesMixin {
public:
  [[nodiscard]] bool isFieldSensitive() const noexcept {
    return hasFlag(AnalysisProperties::FieldSensitive);
  }

  [[nodiscard]] bool isContextSensitive() const noexcept {
    return hasFlag(AnalysisProperties::ContextSensitive);
  }

  [[nodiscard]] bool isFlowSensitive() const noexcept {
    return hasFlag(AnalysisProperties::FlowSensitive);
  }

private:
  [[nodiscard]] bool hasFlag(AnalysisProperties Prop) const noexcept {
    static_assert(std::is_base_of_v<AnalysisPropertiesMixin<Derived>, Derived>,
                  "Invalid CRTP instantiation! Derived must inherit from "
                  "AnalysisPropertiesMixin<Derived>");
    return int(static_cast<const Derived *>(this)->getAnalysisProperties()) &
           int(Prop);
  }
};

} // namespace psr

#endif // PHASAR_UTILS_ANALYSISPROPERTIES_H
