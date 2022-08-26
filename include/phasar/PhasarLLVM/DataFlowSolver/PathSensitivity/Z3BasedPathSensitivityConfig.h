/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYCONFIG_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYCONFIG_H

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityConfig.h"

#include "z3++.h"

#include <optional>

namespace psr {
struct Z3BasedPathSensitivityConfig
    : PathSensitivityConfigBase<Z3BasedPathSensitivityConfig> {
  std::optional<z3::expr> AdditionalConstraint;

  [[nodiscard]] Z3BasedPathSensitivityConfig
  withAdditionalConstraint(const z3::expr &Constr) const &noexcept {
    auto Ret = *this;
    Ret.AdditionalConstraint =
        Ret.AdditionalConstraint ? *Ret.AdditionalConstraint && Constr : Constr;
    return Ret;
  }

  [[nodiscard]] Z3BasedPathSensitivityConfig
  withAdditionalConstraint(const z3::expr &Constr) &&noexcept {
    AdditionalConstraint =
        AdditionalConstraint ? *AdditionalConstraint && Constr : Constr;
    return std::move(*this);
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYCONFIG_H
