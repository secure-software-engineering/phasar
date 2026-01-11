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

#include "phasar/ControlFlow/CFG.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Domain/BinaryDomain.h"

#include "llvm/ADT/STLExtras.h"

#include <functional>
#include <type_traits>

namespace psr {

namespace detail {
template <typename EntryRange, CFG C, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const C &CFG,
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

template <typename EntryRange, CFG C, typename ICFGorIRDB, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const ICFGorIRDB *ICDB,
                          const C &CFG, HandlerFn Handler) {

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

template <typename EntryRange, CFG C, ProjectIRDB DB, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const DB *IRDB,
                          const C &CFG, HandlerFn Handler) {
  return detail::forallStartingPoints(EntryPoints, IRDB, CFG,
                                      std::move(Handler));
}

template <typename EntryRange, typename I, typename HandlerFn>
void forallStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                          HandlerFn Handler) {
  detail::forallStartingPoints(EntryPoints, ICF, *ICF, std::move(Handler));
}

template <typename EntryRange, CFG C, ProjectIRDB DB, typename SeedsT,
          typename D, typename L>
  requires(std::is_convertible_v<L, typename SeedsT::l_t> &&
           std::is_convertible_v<D, typename SeedsT::d_t>)
void addSeedsForStartingPoints(const EntryRange &EntryPoints, const DB *IRDB,
                               const C &CFG, SeedsT &Seeds, const D &ZeroValue,
                               const L &BottomValue) {
  forallStartingPoints(EntryPoints, IRDB, CFG,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
}

template <typename EntryRange, typename I, typename SeedsT, typename D,
          typename L>
  requires(std::is_convertible_v<L, typename SeedsT::l_t> &&
           std::is_convertible_v<D, typename SeedsT::d_t>)
void addSeedsForStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                               SeedsT &Seeds, const D &ZeroValue,
                               const L &BottomValue) {
  forallStartingPoints(EntryPoints, ICF,
                       [&Seeds, &ZeroValue, &BottomValue](const auto &SP) {
                         Seeds.addSeed(SP, ZeroValue, BottomValue);
                       });
}

/// Simplification for IFDS, passing BinaryDomain::BOTTOM as L
template <typename EntryRange, CFG C, ProjectIRDB DB, typename SeedsT,
          typename D>
  requires(std::is_same_v<BinaryDomain, typename SeedsT::l_t> &&
           std::is_convertible_v<D, typename SeedsT::d_t>)
void addSeedsForStartingPoints(const EntryRange &EntryPoints, const DB *IRDB,
                               const C &CFG, SeedsT &Seeds,
                               const D &ZeroValue) {
  addSeedsForStartingPoints(EntryPoints, IRDB, CFG, Seeds, ZeroValue,
                            BinaryDomain::BOTTOM);
}

/// Simplification for IFDS, passing BinaryDomain::BOTTOM as L
template <typename EntryRange, typename I, typename SeedsT, typename D>
  requires(std::is_same_v<BinaryDomain, typename SeedsT::l_t> &&
           std::is_convertible_v<D, typename SeedsT::d_t>)
void addSeedsForStartingPoints(const EntryRange &EntryPoints, const I *ICF,
                               SeedsT &Seeds, const D &ZeroValue) {
  addSeedsForStartingPoints(EntryPoints, ICF, Seeds, ZeroValue,
                            BinaryDomain::BOTTOM);
}
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_ENTRYPOINTUTILS_H
