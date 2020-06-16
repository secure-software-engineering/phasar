/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFACTWRAPPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFACTWRAPPER_H_

#include <memory>
#include <ostream>
#include <type_traits>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFact.h"

namespace psr {

template <typename T> class EdgeFactWrapper : public EdgeFact {
  static_assert(std::is_copy_constructible<T>::value &&
                    std::is_move_constructible<T>::value,
                "The edge fact type must be copy- and move constructible");
  T fact;

public:
  using l_t = T;
  EdgeFactWrapper(const T &f) : fact(f) {}
  EdgeFactWrapper(T &&f) : fact(std::move(f)) {}
  virtual ~EdgeFactWrapper() = default;
  const T &get() const { return fact; }
  void print(std::ostream &os) const override { os << fact << '\n'; }
};

template <typename EFW> class EdgeFactManager {
  static_assert(std::is_base_of<EdgeFactWrapper<typename EFW::l_t>, EFW>::value,
                "Your custom EdgeFactWrapper type must inherit from "
                "psr::EdgeFactWrapper");
  static_assert(!std::is_abstract<EFW>::value,
                "Your custom EdgeFactWrapper is an abstract class. Please make "
                "sure to overwrite all pure virtual functions");
  static_assert(
      std::is_same<EFW *,
                   decltype(new EFW(std::declval<typename EFW::l_t>()))>::value,
      "Your custom EdgeFactWrapper must have a constructor where the only "
      "parameter is of the wrapped type l_t");
  std::map<typename EFW::l_t, std::unique_ptr<EFW>> cache;

public:
  EdgeFact *getOrCreateEdgeFact(const typename EFW::l_t &fact) {
    auto it = cache.find(fact);
    if (it != cache.end())
      return it->second.get();

    auto ret = new EFW(fact);
    cache[fact] = std::unique_ptr<EFW>(ret);
    return ret;
  }
  EdgeFact *getOrCreateEdgeFact(typename EFW::l_t &&fact) {
    auto it = cache.find(fact);
    if (it != cache.end())
      return it->second.get();

    auto ret = new EFW(fact);
    cache[std::move(fact)] = std::unique_ptr<EFW>(ret);
    return ret;
  }
};

} // namespace psr

#endif
