/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H
#define PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H

#include "phasar/Utils/ByRef.h"

#include <type_traits>

namespace psr {
template <typename Derived, typename F, typename V> class SparseCFGProvider {
public:
  using f_t = F;
  using v_t = V;

  template <typename D>
  [[nodiscard]] decltype(auto) getSparseCFG(ByConstRef<f_t> Fun,
                                            const D &Val) const {
    static_assert(std::is_convertible_v<decltype(valueOf(Val)), v_t>);
    return self().getSparseCFGImpl(Fun, valueOf(Val));
  }

private:
  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

} // namespace psr

#endif // PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H
