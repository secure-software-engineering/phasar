/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFACTWRAPPER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFACTWRAPPER_H

#include <map>
#include <memory>
#include <ostream>
#include <type_traits>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFact.h"

namespace psr {

/// A Wrapper over your edge-fact. Please create a subclass of this template and
/// overwrite the print function if necessary
template <typename T> class EdgeFactWrapper : public EdgeFact {
  static_assert(std::is_copy_constructible_v<T> &&
                    std::is_move_constructible_v<T>,
                "The edge fact type must be copy- and move constructible");

private:
  T Fact;

public:
  using l_t = T;
  EdgeFactWrapper(const T &F) : Fact(F) {}
  EdgeFactWrapper(T &&F) : Fact(std::move(F)) {}
  ~EdgeFactWrapper() override = default;
  [[nodiscard]] const T &get() const { return Fact; }
  void print(std::ostream &OS) const override { OS << Fact << '\n'; }
};

/// A simple memory manager for EdgeFactWrappers. You may use them in your
/// IDETabulationProblem to manage your edge-facts. Please note that this
/// manager only works, if the template argument EFW is a non-abstract (subclass
/// of) EdgeFactWrapper and has a constructor taking a single edge-fact.
template <typename EFW> class EdgeFactManager {
  static_assert(std::is_base_of_v<EdgeFactWrapper<typename EFW::l_t>, EFW>,
                "Your custom EdgeFactWrapper type must inherit from "
                "psr::EdgeFactWrapper");
  static_assert(!std::is_abstract_v<EFW>,
                "Your custom EdgeFactWrapper is an abstract class. Please make "
                "sure to overwrite all pure virtual functions");
  static_assert(
      std::is_same_v<EFW *,
                     decltype(new EFW(std::declval<typename EFW::l_t>()))>,
      "Your custom EdgeFactWrapper must have a constructor where the only "
      "parameter is of the wrapped type l_t");

private:
  std::map<typename EFW::l_t, std::unique_ptr<EFW>> Cache;

public:
  template <typename EFTy = typename EFW::l_t>
  EdgeFact *getOrCreateEdgeFact(EFTy &&Fact) {
    auto &CValue = Cache[Fact];
    if (!CValue) {
      CValue = std::make_unique<EFW>(std::forward<EFTy>(Fact));
    }
    return CValue.get();
  }
};

} // namespace psr

#endif
