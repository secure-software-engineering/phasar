/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_ENTRYPOINTUTILS_H
#define PHASAR_DATAFLOW_IFDSIDE_ENTRYPOINTUTILS_H

#include "phasar/DB/ProjectIRDBBase.h"

#include <functional>

namespace psr {

template <typename EntryRange, typename C, typename FunctionRetriever,
          typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints,
                          FunctionRetriever GetFunction, const C &CFG,
                          HandlerFn Handler) {
  for (const auto &EntryPoint : EntryPoints) {
    const auto &Fn = std::invoke(GetFunction, EntryPoint);
    if constexpr (std::is_convertible_v<std::decay_t<decltype(Fn)>, bool>) {
      if (!Fn) {
        continue;
      }
    }
    for (const auto &SP : CFG.getStartPointsOf(Fn)) {
      std::invoke(Handler, SP);
    }
  }
}

template <typename EntryRange, typename C, typename FunctionRetriever,
          typename SeedsT, typename D, typename L>
void addSeedsForStartingPoints(const EntryRange &EntryPoints,
                               FunctionRetriever GetFunction, const C &CFG,
                               SeedsT &Seeds, const D &ZeroValue,
                               const L &BottomValue) {
  forallStartingPoints(EntryPoints, std::move(GetFunction), CFG,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
}

template <typename EntryRange, typename C, typename DB, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints,
                          const ProjectIRDBBase<DB> *IRDB, const C &CFG,
                          HandlerFn Handler) {
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    forallStartingPoints(
        IRDB->getAllFunctions(), [](auto Fn) { return Fn; }, CFG,
        std::move(Handler));
  } else {
    forallStartingPoints(EntryPoints, IRDBGetFunctionDef(IRDB), CFG,
                         std::move(Handler));
  }
}

template <typename EntryRange, typename C, typename DB, typename SeedsT,
          typename D, typename L>
void addSeedsForStartingPoints(const EntryRange &EntryPoints,
                               const ProjectIRDBBase<DB> *IRDB, const C &CFG,
                               SeedsT &Seeds, const D &ZeroValue,
                               const L &BottomValue) {
  forallStartingPoints(EntryPoints, IRDB, CFG,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
}

template <typename EntryRange, typename I, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                          HandlerFn Handler) {
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    forallStartingPoints(
        ICF->getAllFunctions(), [](auto Fn) { return Fn; }, *ICF,
        std::move(Handler));
  } else {
    forallStartingPoints(
        EntryPoints,
        [ICF](llvm::StringRef Name) { return ICF->getFunction(Name); }, *ICF,
        std::move(Handler));
  }
}

template <typename EntryRange, typename I, typename SeedsT, typename D,
          typename L>
void addSeedsForStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                               SeedsT &Seeds, const D &ZeroValue,
                               const L &BottomValue) {
  forallStartingPoints(EntryPoints, ICF,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
}
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_ENTRYPOINTUTILS_H
