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

  [[nodiscard]] decltype(auto) getAllFunctions() const {
    return self().getAllFunctionsImpl();
  }

  [[nodiscard]] f_t getFunction(llvm::StringRef Fun) const {
    return self().getFunctionImpl(Fun);
  }

  [[nodiscard]] bool isIndirectFunctionCall(n_t Inst) const {
    return self().isIndirectFunctionCallImpl(Inst);
  }
  [[nodiscard]] bool isVirtualFunctionCall(n_t Inst) const {
    return self().isVirtualFunctionCallImpl(Inst);
  }
  [[nodiscard]] decltype(auto) allNonCallStartNodes() const {
    static_assert(
        is_iterable_over_v<decltype(self().allNonCallStartNodesImpl()), n_t>);
    return self().allNonCallStartNodesImpl();
  }
  [[nodiscard]] decltype(auto) getCalleesOfCallAt(n_t Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCalleesOfCallAtImpl(Inst)), f_t>);
    return self().getCalleesOfCallAtImpl(Inst);
  }
  [[nodiscard]] decltype(auto) getCallersOf(f_t Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCallersOfImpl(Fun)), n_t>);
    return self().getCallersOfImpl(Fun);
  }
  [[nodiscard]] decltype(auto) getCallsFromWithin(f_t Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCallsFromWithinImpl(Fun)), n_t>);
    return self().getCallsFromWithinImpl(Fun);
  }
  [[nodiscard]] decltype(auto) getReturnSitesOfCallAt(n_t Inst) const {
    static_assert(
        is_iterable_over_v<decltype(self().getReturnSitesOfCallAtImpl(Inst)),
                           n_t>);
    return self().getReturnSitesOfCallAtImpl(Inst);
  }
  [[nodiscard]] decltype(auto) getGlobalInitializers(f_t Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getGlobalInitializersImpl(Fun)),
                           f_t>);
    return self().getGlobalInitializersImpl(Fun);
  }
  void print(llvm::raw_ostream &OS = llvm::outs()) const {
    self().printImpl(OS);
  }
  [[nodiscard]] nlohmann::json getAsJson() const {
    return self().getAsJsonImpl();
  }

private:
  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

template <typename ICF, typename Domain>
constexpr bool is_icfg_v = is_crtp_base_of_v<ICFGBase, ICF>
    &&std::is_same_v<typename ICF::n_t, typename Domain::n_t>
        &&std::is_same_v<typename ICF::f_t, typename Domain::f_t>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_ICFGBASE_H
