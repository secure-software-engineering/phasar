/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFACTWRAPPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFACTWRAPPER_H_

#include <iostream>
#include <optional>
#include <set>
#include <type_traits>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFact.h"

namespace psr {

/// A Wrapper over your dataflow-fact. It already contains a special treatment
/// for the ZERO (Λ) value. Please create a subclass of this template and
/// overwrite the print function if necessary. Note, that yout dataflow-fact
/// must be copy- and move constructible
template <typename T> class FlowFactWrapper : public FlowFact {
  static_assert(std::is_copy_constructible_v<T> &&
                    std::is_move_constructible_v<T>,
                "The dataflow fact type must be copy- and move constructible");

private:
  std::optional<T> Fact;

public:
  using d_t = T;

  FlowFactWrapper(std::nullptr_t) : Fact() {}
  FlowFactWrapper(const T &F) : Fact(F) {}
  FlowFactWrapper(T &&F) : Fact(std::move(F)) {}
  ~FlowFactWrapper() override = default;
  const std::optional<T> &get() const { return Fact; }
  bool isZero() const { return !Fact; }

  void print(std::ostream &OS) const override final {
    if (isZero()) {
      OS << "Λ";
    } else {
      print(OS, *Fact);
    }
    OS << '\n';
  }

  virtual void print(std::ostream &OS, const T &NonZeroFact) const {
    OS << NonZeroFact;
  }
};

/// A simple memory manager for FlowFactWrappers. You may use them in your
/// TabulationProblem to manage your dataflow-facts. Please note that this
/// manager only works, if the template argument FFW is a non-abstract (subclass
/// of) FlowFactWrapper and has a constructor taking a single dataflow-fact (or
/// nullptr for ZERO).
template <typename FFW> class FlowFactManager {
  static_assert(std::is_base_of_v<FlowFactWrapper<typename FFW::d_t>, FFW>,
                "Your custom FlowFactWrapper type must inherit from "
                "psr::FlowFactWrapper");
  static_assert(!std::is_abstract_v<FFW>,
                "Your custom FlowFactWrapper is an abstract class. Please make "
                "sure to overwrite all pure virtual functions");
  static_assert(
      std::is_same_v<FFW *,
                     decltype(new FFW(std::declval<typename FFW::d_t>()))>,
      "Your custom FlowFactWrapper must have a constructor where the only "
      "parameter is of the wrapped type d_t");
  static_assert(std::is_same_v<FFW *, decltype(new FFW(nullptr))>,
                "Your custom FlowFactWrapper must have a constructor that "
                "takes a nullptr for creating the zero value");

private:
  std::map<typename FFW::d_t, std::unique_ptr<FFW>> Cache;
  std::unique_ptr<FFW> ZeroCache;

  // Allow the 'getOrCreateFlowFacts' template to get FlowFacts passed by using
  // this overload
  const FlowFact *getOrCreateFlowFact(const FlowFact *Fact) const {
    return Fact;
  }

public:
  FlowFact *getOrCreateZero() {
    if (!ZeroCache) {
      ZeroCache = std::make_unique<FFW>(nullptr);
    }
    return ZeroCache.get();
  }

  template <typename FFTy = typename FFW::d_t>
  FlowFact *getOrCreateFlowFact(FFTy &&Fact) {
    auto &CValue = Cache[Fact];
    if (!CValue) {
      CValue = std::make_unique<FFW>(std::forward<FFTy>(Fact));
    }
    return CValue.get();
  }

  template <typename... Args>
  std::set<const FlowFact *> getOrCreateFlowFacts(Args &&... args) {
    std::set<const FlowFact *> Ret;
    (Ret.insert(getOrCreateFlowFact(std::forward<Args>(args))), ...);
    return Ret;
  }
};
} // namespace psr

#endif
