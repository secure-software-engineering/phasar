/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_SPARSECFGBASE_H
#define PHASAR_CONTROLFLOW_SPARSECFGBASE_H

#include "phasar/ControlFlow/CFGBase.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Nullable.h"

namespace psr {
template <typename Derived> class SparseCFGBase : public CFGBase<Derived> {
public:
  using typename CFGBase<Derived>::n_t;
  using typename CFGBase<Derived>::f_t;

  /// Gets the next instruction in control-flow order, starting from
  /// FromInstruction, that may use or define Val.
  /// If the next user is ambiguous, returns null.
  [[nodiscard]] Nullable<n_t>
  nextUserOrNull(ByConstRef<n_t> FromInstruction) const {
    return self().nextUserOrNullImpl(FromInstruction);
  }

protected:
  using CFGBase<Derived>::self;
};

template <typename ICF, typename Domain>
// NOLINTNEXTLINE(readability-identifier-naming)
constexpr bool is_sparse_cfg_v = is_crtp_base_of_v<SparseCFGBase, ICF>
    &&std::is_same_v<typename ICF::n_t, typename Domain::n_t>
        &&std::is_same_v<typename ICF::f_t, typename Domain::f_t>;

} // namespace psr

#endif // PHASAR_CONTROLFLOW_SPARSECFGBASE_H
