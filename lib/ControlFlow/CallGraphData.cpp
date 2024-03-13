/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphData.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {
  void CallGraphData::printAsJson(llvm::raw_ostream &OS) {
    nlohmann::json JSON;

    for (const auto &CurrentCallerOf : CallersOf) {
      for (const auto &CurrentElement : CurrentCallerOf.FToFunctionVertexTy) {
        for (const auto &NTValString : CurrentElement.second) {
          JSON[CurrentElement.first].push_back(NTValString);
        }
      }
    }

    OS << JSON;
  }
} // namespace psr
