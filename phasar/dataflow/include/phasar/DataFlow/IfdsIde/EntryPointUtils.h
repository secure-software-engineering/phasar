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

#include "phasar/ControlFlow/CFGBase.h"
#include "phasar/DB/ProjectIRDBBase.h"

#include "llvm/ADT/STLExtras.h"

#include <functional>

namespace psr {

namespace detail {
template <typename EntryRange, typename C, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const CFGBase<C> &CFG,
                          HandlerFn Handler) {
  for (const auto &EntryPointFn : EntryPoints) {
    if constexpr (std::is_convertible_v<std::decay_t<decltype(EntryPointFn)>,
                                        bool>) {
      if (!EntryPointFn) {
        continue;
      }
    }
    for (const auto &SP : CFG.getStartPointsOf(EntryPointFn)) {
      std::invoke(Handler, SP);
    }
  }
}

template <typename EntryRange, typename C, typename ICFGorIRDB,
          typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const ICFGorIRDB *ICDB,
                          const CFGBase<C> &CFG, HandlerFn Handler) {

  if (llvm::hasSingleElement(EntryPoints) &&
      *llvm::adl_begin(EntryPoints) == "__ALL__") {
    forallStartingPoints(ICDB->getAllFunctions(), CFG, std::move(Handler));
  } else {
    forallStartingPoints(llvm::map_range(EntryPoints,
                                         [ICDB](llvm::StringRef Name) {
                                           return ICDB->getFunction(Name);
                                         }),
                         CFG, std::move(Handler));
  }
}

} // namespace detail

template <typename EntryRange, typename C, typename DB, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints,
                          const ProjectIRDBBase<DB> *IRDB,
                          const CFGBase<C> &CFG, HandlerFn Handler) {
  return detail::forallStartingPoints(EntryPoints, IRDB, CFG,
                                      std::move(Handler));
}

template <typename EntryRange, typename I, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                          HandlerFn Handler) {
  detail::forallStartingPoints(EntryPoints, ICF, *ICF, std::move(Handler));
}

template <typename EntryRange, typename C, typename DB, typename SeedsT,
          typename D, typename L>
void addSeedsForStartingPoints(const EntryRange &EntryPoints,
                               const ProjectIRDBBase<DB> *IRDB,
                               const CFGBase<C> &CFG, SeedsT &Seeds,
                               const D &ZeroValue, const L &BottomValue) {
  forallStartingPoints(EntryPoints, IRDB, CFG,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
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
