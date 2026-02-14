/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/
#pragma once

#include "phasar/ControlFlow/CallGraphBase.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>

namespace psr {
template <typename T>
concept ICFG = requires(const T &ICF, llvm::StringRef Name,
                        typename T::n_t Inst, typename T::f_t Fun) {
  typename T::f_t;
  typename T::n_t;

  // TODO: Should not be duplicated with ProjectIRDB
  { ICF.getAllFunctions() } -> is_iterable_over_v<typename T::f_t>;
  // TODO: Should not be duplicated with ProjectIRDB
  { ICF.getFunction(Name) } -> std::convertible_to<Nullable<typename T::f_t>>;

  { ICF.isIndirectFunctionCall(Inst) } -> std::convertible_to<bool>;
  { ICF.isVirtualFunctionCall(Inst) } -> std::convertible_to<bool>;

  /// Gets the underlying call-graph
  { ICF.getCallGraph() } -> psr::IsCallGraphRef;
  /// Returns an iterable range of all possible callee candidates at the given
  /// call-site induced by the used call-graph. Same as
  /// getCallGraph().getCalleesOfCallAt(Inst)
  { ICF.getCalleesOfCallAt(Inst) } -> psr::is_iterable_over_v<typename T::f_t>;
  /// Returns an iterable range of all possible call-site candidates that may
  /// call the given function induced by the used call-graph. Same as
  /// getCallGraph().getCallersOf(Fun)
  { ICF.getCallersOf(Fun) } -> psr::is_iterable_over_v<typename T::n_t>;

  /// Returns an iterable range of all call-instruction in the given function
  { ICF.getCallsFromWithin(Fun) } -> psr::is_iterable_over_v<typename T::n_t>;

  /// Returns an iterable range of all instructions in all functions of the ICFG
  /// that are neither call-sites nor start-points of a function
  // TODO: Get rid of this function
  { ICF.allNonCallStartNodes() } -> psr::is_iterable_over_v<typename T::n_t>;

  /// The total number of call-sites in the ICFG. Same as
  /// getCallGraph().getNumVertexCallSites()
  { ICF.getNumCallSites() } noexcept -> std::convertible_to<size_t>;
};

template <typename T>
concept ICFGDump = requires(const T &ICF, llvm::raw_ostream &OS) {
  ICF.print(OS);
  ICF.printAsJson(OS);
};
} // namespace psr
