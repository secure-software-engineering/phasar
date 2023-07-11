/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_CFGBASE_H
#define PHASAR_CONTROLFLOW_CFGBASE_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "nlohmann/json.hpp"

namespace psr {

enum class SpecialMemberFunctionType;

template <typename T> struct CFGTraits {
  // using n_t
  // using f_t
};

template <typename Derived> class CFGBase {
public:
  using n_t = typename CFGTraits<Derived>::n_t;
  using f_t = typename CFGTraits<Derived>::f_t;

  /// Returns the function where the given instruction is defined
  [[nodiscard]] f_t getFunctionOf(ByConstRef<n_t> Inst) const noexcept {
    return self().getFunctionOfImpl(Inst);
  }
  /// Returns an iterable range of all predecessor instructions of Inst in the
  /// CFG
  [[nodiscard]] decltype(auto) getPredsOf(ByConstRef<n_t> Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getPredsOfImpl(Inst)), n_t>);
    return self().getPredsOfImpl(Inst);
  }
  /// Returns an iterable range of all successor instructions of Inst in the
  /// CFG. NOTE: This function is typically being called in a hot part of the
  /// analysis and should therefore be highly optimized for performance
  [[nodiscard]] decltype(auto) getSuccsOf(ByConstRef<n_t> Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getSuccsOfImpl(Inst)), n_t>);
    return self().getSuccsOfImpl(Inst);
  }
  /// Returns an iterable range of all edges in the CFG of the given function
  [[nodiscard]] decltype(auto)
  getAllControlFlowEdges(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getAllControlFlowEdgesImpl(Fun)),
                           std::pair<n_t, n_t>>);
    return self().getAllControlFlowEdgesImpl(Fun);
  }
  /// Returns an iterable range of all instructions of the given function. NOTE:
  /// even if the CFG is initialized to ignore debugging instructions, they may
  /// be contained here.
  [[nodiscard]] decltype(auto) getAllInstructionsOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getAllInstructionsOfImpl(Fun)),
                           n_t>);
    return self().getAllInstructionsOfImpl(Fun);
  }
  /// Returns an iterable range of all starting instructions of the given
  /// function. For a forward-CFG, this is typically a singleton range
  [[nodiscard]] decltype(auto) getStartPointsOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getStartPointsOfImpl(Fun)), n_t>);
    return self().getStartPointsOfImpl(Fun);
  }
  /// Returns an iterable range of all exit instructions (often return
  /// instructions) of the given function. For a backward-CFG, this is typically
  /// a singleton range
  [[nodiscard]] decltype(auto) getExitPointsOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getExitPointsOfImpl(Fun)), n_t>);
    return self().getExitPointsOfImpl(Fun);
  }
  [[nodiscard]] bool isCallSite(ByConstRef<n_t> Inst) const noexcept {
    return self().isCallSiteImpl(Inst);
  }
  [[nodiscard]] bool isExitInst(ByConstRef<n_t> Inst) const noexcept {
    return self().isExitInstImpl(Inst);
  }
  [[nodiscard]] bool isStartPoint(ByConstRef<n_t> Inst) const noexcept {
    return self().isStartPointImpl(Inst);
  }
  [[nodiscard]] bool isFieldLoad(ByConstRef<n_t> Inst) const noexcept {
    return self().isFieldLoadImpl(Inst);
  }
  [[nodiscard]] bool isFieldStore(ByConstRef<n_t> Inst) const noexcept {
    return self().isFieldStoreImpl(Inst);
  }
  [[nodiscard]] bool
  isFallThroughSuccessor(ByConstRef<n_t> Inst,
                         ByConstRef<n_t> Succ) const noexcept {
    return self().isFallThroughSuccessorImpl(Inst, Succ);
  }
  [[nodiscard]] bool isBranchTarget(ByConstRef<n_t> Inst,
                                    ByConstRef<n_t> Succ) const noexcept {
    return self().isBranchTargetImpl(Inst, Succ);
  }
  [[nodiscard]] bool isHeapAllocatingFunction(ByConstRef<f_t> Fun) const {
    return self().isHeapAllocatingFunctionImpl(Fun);
  }
  [[nodiscard]] bool isSpecialMemberFunction(ByConstRef<f_t> Fun) const {
    return self().isSpecialMemberFunctionImpl(Fun);
  }
  [[nodiscard]] SpecialMemberFunctionType
  getSpecialMemberFunctionType(ByConstRef<f_t> Fun) const {
    return self().getSpecialMemberFunctionTypeImpl(Fun);
  }
  [[nodiscard]] decltype(auto) getStatementId(ByConstRef<n_t> Inst) const {
    static_assert(is_string_like_v<decltype(self().getStatementIdImpl(Inst))>);
    return self().getStatementIdImpl(Inst);
  }
  [[nodiscard]] decltype(auto) getFunctionName(ByConstRef<f_t> Fun) const {
    static_assert(is_string_like_v<decltype(self().getFunctionNameImpl(Fun))>);
    return self().getFunctionNameImpl(Fun);
  }
  [[nodiscard]] decltype(auto)
  getDemangledFunctionName(ByConstRef<f_t> Fun) const {
    static_assert(
        is_string_like_v<decltype(self().getDemangledFunctionNameImpl(Fun))>);
    return self().getDemangledFunctionNameImpl(Fun);
  }
  void print(ByConstRef<f_t> Fun, llvm::raw_ostream &OS) const {
    self().printImpl(Fun, OS);
  }
  [[nodiscard]] nlohmann::json getAsJson(ByConstRef<f_t> Fun) const {
    return self().getAsJsonImpl(Fun);
  }

private:
  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

template <typename ICF, typename Domain>
// NOLINTNEXTLINE(readability-identifier-naming)
constexpr bool is_cfg_v = is_crtp_base_of_v<CFGBase, ICF>
    &&std::is_same_v<typename ICF::n_t, typename Domain::n_t>
        &&std::is_same_v<typename ICF::f_t, typename Domain::f_t>;

} // namespace psr

#endif // PHASAR_CONTROLFLOW_CFGBASE_H
