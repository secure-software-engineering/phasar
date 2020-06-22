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
  static_assert(std::is_copy_constructible<T>::value &&
                    std::is_move_constructible<T>::value,
                "The dataflow fact type must be copy- and move constructible");
  std::optional<T> fact;

public:
  using d_t = T;

  FlowFactWrapper(std::nullptr_t) : fact() {}
  FlowFactWrapper(const T &f) : fact(f) {}
  FlowFactWrapper(T &&f) : fact(std::move(f)) {}
  ~FlowFactWrapper() override = default;
  const std::optional<T> &get() const { return fact; }
  bool isZero() const { return !fact; }

  void print(std::ostream &os) const override final {
    if (isZero())
      os << "Λ";
    else
      print(os, *fact);

    os << '\n';
  }

  virtual void print(std::ostream &os, const T &nonzeroFact) const {
    os << nonzeroFact;
  }
};

/// A simple memory manager for FlowFactWrappers. You may use them in your
/// TabulationProblem to manage your dataflow-facts. Please note that this
/// manager only works, if the template argument FFW is a non-abstract (subclass
/// of) FlowFactWrapper and has a constructor taking a single dataflow-fact.
template <typename FFW> class FlowFactManager {
  static_assert(std::is_base_of<FlowFactWrapper<typename FFW::d_t>, FFW>::value,
                "Your custom FlowFactWrapper type must inherit from "
                "psr::FlowFactWrapper");
  static_assert(!std::is_abstract<FFW>::value,
                "Your custom FlowFactWrapper is an abstract class. Please make "
                "sure to overwrite all pure virtual functions");
  static_assert(
      std::is_same<FFW *,
                   decltype(new FFW(std::declval<typename FFW::d_t>()))>::value,
      "Your custom FlowFactWrapper must have a constructor where the only "
      "parameter is of the wrapped type d_t");
  std::map<typename FFW::d_t, std::unique_ptr<FFW>> cache;
  std::unique_ptr<FFW> zeroCache;

  // Allow the 'getOrCreateFlowFacts' template to get FlowFacts passed by using
  // this overload
  const FlowFact *getOrCreateFlowFact(const FlowFact *fact) const {
    return fact;
  }

public:
  FlowFact *getOrCreateZero() {
    if (!zeroCache)
      zeroCache = std::make_unique<FFW>(nullptr);

    return zeroCache.get();
  }
  FlowFact *getOrCreateFlowFact(const typename FFW::d_t &fact) {
    auto &cValue = cache[fact];
    if (!cValue)
      cValue = std::make_unique<FFW>(fact);
    return cValue.get();
  }
  FlowFact *getOrCreateFlowFact(typename FFW::d_t &&fact) {
    auto &cValue = cache[fact];
    if (!cValue)
      cValue = std::make_unique<FFW>(std::move(fact));
    return cValue.get();
  }

  template <typename... Args>
  std::set<const FlowFact *> getOrCreateFlowFacts(Args &&... args) {
    std::set<const FlowFact *> ret;
    (ret.insert(getOrCreateFlowFact(std::forward<Args>(args))), ...);
    return ret;
  }
};
} // namespace psr

#endif
