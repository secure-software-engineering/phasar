/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_ICFGBASE_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_ICFGBASE_H

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/Utils/TypeTraits.h"

#include "nlohmann/json.hpp"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {
template <typename Derived> class ICFGBase {
public:
  using n_t = typename CFGTraits<Derived>::n_t;
  using f_t = typename CFGTraits<Derived>::f_t;

  ICFGBase() noexcept {
    static_assert(is_crtp_base_of_v<CFGBase, Derived>,
                  "An ICFG must also be a CFG");
  }

  /// Returns an iterable range of all function definitions or declarations in
  /// the ICFG
  [[nodiscard]] decltype(auto) getAllFunctions() const {
    return self().getAllFunctionsImpl();
  }

  /// returns the function definition or declaration with the given name. If
  /// ther eis no such function, returns a default constructed f_t (nullptr for
  /// pointers).
  [[nodiscard]] f_t getFunction(llvm::StringRef Fun) const {
    return self().getFunctionImpl(Fun);
  }

  /// Returns true, iff the given instruction is a call-site where the callee is
  /// called via a function-pointer or another kind of virtual call.
  /// NOTE: Trivial cases where a bitcast of a function is called may still
  /// count as indirect call.
  [[nodiscard]] bool isIndirectFunctionCall(ByConstRef<n_t> Inst) const {
    return self().isIndirectFunctionCallImpl(Inst);
  }
  /// Returns true, iff the given instruction is a call-site where the callee is
  /// called via virtual dispatch. NOTE: Here, a class-hierarchy is required and
  /// a simple function-pointer is not sufficient.
  [[nodiscard]] bool isVirtualFunctionCall(ByConstRef<n_t> Inst) const {
    return self().isVirtualFunctionCallImpl(Inst);
  }
  /// Returns an iterable range of all instructions in all functions of the ICFG
  /// that are neither call-sites nor start-points of a function
  [[nodiscard]] decltype(auto) allNonCallStartNodes() const {
    static_assert(
        is_iterable_over_v<decltype(self().allNonCallStartNodesImpl()), n_t>);
    return self().allNonCallStartNodesImpl();
  }
  /// Returns an iterable range of all possible callee candidates at the given
  /// call-site induced by the used call-graph. NOTE: This function is typically
  /// called in a hot part of the analysis and should therefore be very fast
  [[nodiscard]] decltype(auto) getCalleesOfCallAt(ByConstRef<n_t> Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCalleesOfCallAtImpl(Inst)), f_t>);
    return self().getCalleesOfCallAtImpl(Inst);
  }
  /// Returns an iterable range of all possible call-site candidates that may
  /// call the given function induced by the used call-graph.
  [[nodiscard]] decltype(auto) getCallersOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCallersOfImpl(Fun)), n_t>);
    return self().getCallersOfImpl(Fun);
  }
  /// Returns an iterable range of all call-instruction in the given function
  [[nodiscard]] decltype(auto) getCallsFromWithin(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCallsFromWithinImpl(Fun)), n_t>);
    return self().getCallsFromWithinImpl(Fun);
  }
  /// Returns an iterable range of all return-sites of the given
  /// call-instruction. Often the same as getSuccsOf. NOTE: This function is
  /// typically called in a hot part of the analysis and should therefore be
  /// very fast
  [[nodiscard]] decltype(auto)
  getReturnSitesOfCallAt(ByConstRef<n_t> Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getReturnSitesOfCallAtImpl(Inst)),
                           n_t>);
    return self().getReturnSitesOfCallAtImpl(Inst);
  }
  /// Returns an iterable range of all global initializer functions
  [[nodiscard]] decltype(auto)
  getGlobalInitializers(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getGlobalInitializersImpl(Fun)),
                           f_t>);
    return self().getGlobalInitializersImpl(Fun);
  }
  /// Prints the underlying call-graph as DOT to the given output-stream
  void print(llvm::raw_ostream &OS = llvm::outs()) const {
    self().printImpl(OS);
  }
  /// Returns the underlying call-graph as JSON
  [[nodiscard]] nlohmann::json getAsJson() const {
    return self().getAsJsonImpl();
  }

private:
  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

/// True, iff ICF is a proper instantiation of ICFGBase with n_t and f_t taken
/// from the given analysis-Domain
template <typename ICF, typename Domain>
// NOLINTNEXTLINE(readability-identifier-naming)
constexpr bool is_icfg_v = is_crtp_base_of_v<ICFGBase, ICF>
    &&std::is_same_v<typename ICF::n_t, typename Domain::n_t>
        &&std::is_same_v<typename ICF::f_t, typename Domain::f_t>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_ICFGBASE_H
