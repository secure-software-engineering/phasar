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

#include "phasar/Utils/EnumFlags.h"

#include "llvm/Support/raw_ostream.h"

#include <string>

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

} // namespace psr

#endif // PHASAR_UTILS_ANALYSISPROPERTIES_H
