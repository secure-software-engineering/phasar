/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/
#pragma once

#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace psr {

template <typename T>
concept InstructionClassifier =
    requires(const T &IC, typename std::remove_cvref_t<T>::n_t Inst,
             typename std::remove_cvref_t<T>::n_t Succ) {
      { IC.isCallSite(Inst) } -> std::convertible_to<bool>;
      { IC.isFieldLoad(Inst) } -> std::convertible_to<bool>;
      { IC.isFieldStore(Inst) } -> std::convertible_to<bool>;
      { IC.isFallThroughSuccessor(Inst, Succ) } -> std::convertible_to<bool>;
      { IC.isBranchTarget(Inst, Succ) } -> std::convertible_to<bool>;
    };

template <typename T>
concept CFG = requires(const T &CF, typename std::remove_cvref_t<T>::n_t Inst,
                       typename std::remove_cvref_t<T>::f_t Fun) {
  typename std::remove_cvref_t<T>::n_t;
  typename std::remove_cvref_t<T>::f_t;

  /// Returns the function that contains the given instruction Inst.
  // TODO: Actually belongs into ProjectIRDB!
  {
    CF.getFunctionOf(Inst)
  } -> std::convertible_to<typename std::remove_cvref_t<T>::f_t>;
  /// Returns an iterable range of all instructions of the given function that
  /// are part of the control-flow graph.
  // TODO: We should have sth like this in the ProjectIRDB as well!
  {
    CF.getAllInstructionsOf(Fun)
  } -> psr::is_iterable_over_v<typename std::remove_cvref_t<T>::n_t>;

  /// Returns an iterable range of all successor instructions of Inst in the
  /// CFG.
  /// NOTE: This function is typically being called in a hot part of the
  /// analysis and should therefore be highly optimized for performance.
  {
    CF.getSuccsOf(Inst)
  } -> psr::is_iterable_over_v<typename std::remove_cvref_t<T>::n_t>;

  /// Returns an iterable range of all starting instructions of the given
  /// function. For a forward-CFG, this is typically a singleton range.
  {
    CF.getStartPointsOf(Fun)
  } -> psr::is_iterable_over_v<typename std::remove_cvref_t<T>::n_t>;

  /// Returns whether the given Inst is a root node of the CFG
  { CF.isStartPoint(Inst) } -> std::convertible_to<bool>;

  /// Returns whether the given Inst is a leaf node of the CFG
  { CF.isExitInst(Inst) } -> std::convertible_to<bool>;

  requires InstructionClassifier<T>;
};

template <typename T, typename N, typename F>
concept CFGOf =
    CFG<T> && std::same_as<N, typename std::remove_cvref_t<T>::n_t> &&
    std::same_as<F, typename std::remove_cvref_t<T>::f_t>;

template <typename T>
concept BidiCFG =
    CFG<T> && requires(const T &CF, typename std::remove_cvref_t<T>::n_t Inst,
                       typename std::remove_cvref_t<T>::f_t Fun) {
      /// Returns an iterable range of all predecessor instructions of Inst in
      /// the CFG
      {
        CF.getPredsOf(Inst)
      } -> psr::is_iterable_over_v<typename std::remove_cvref_t<T>::n_t>;

      /// Returns an iterable range of all exit instructions (often return
      /// instructions) of the given function. For a backward-CFG, this is
      /// typically a singleton range
      {
        CF.getExitPointsOf(Fun)
      } -> psr::is_iterable_over_v<typename std::remove_cvref_t<T>::n_t>;
    };

template <typename T>
concept CFGDump =
    requires(const T &CF, typename std::remove_cvref_t<T>::n_t Inst,
             typename std::remove_cvref_t<T>::f_t Fun, llvm::raw_ostream &OS) {
      { CF.getStatementId(Inst) } -> psr::is_string_like_v;
      { CF.getFunctionName(Fun) } -> psr::is_string_like_v;
      { CF.getDemangledFunctionName(Fun) } -> psr::is_string_like_v;
      CF.print(Fun, OS);
    };

template <typename T>
concept CFGEdgesProvider = requires(const T &CF,
                                    typename std::remove_cvref_t<T>::f_t Fun) {
  {
    CF.getAllControlFlowEdges(Fun)
  } -> psr::is_iterable_over_v<std::pair<typename std::remove_cvref_t<T>::n_t,
                                         typename std::remove_cvref_t<T>::n_t>>;
};

template <typename T>
concept IsBlockAwareControlFlow =
    requires(const T &CF, typename std::remove_cvref_t<T>::n_t Inst) {
      {
        CF.getUniqueSuccessor(Inst)
      } -> std::convertible_to<Nullable<typename std::remove_cvref_t<T>::n_t>>;
      { CF.hasUniquePredecessor(Inst) } -> std::convertible_to<bool>;
    };
} // namespace psr
