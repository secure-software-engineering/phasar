/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYCONFIG_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYCONFIG_H

#include <cstddef>
#include <cstdint>

namespace psr {

template <typename DerivedConfig> struct PathSensitivityConfigBase {
  size_t DAGSizeThreshold = SIZE_MAX;
  size_t DAGDepthThreshold = SIZE_MAX;
  size_t NumPathsThreshold = SIZE_MAX;
  bool MinimizeDAG = true;

  [[nodiscard]] DerivedConfig
  withDAGSizeThreshold(size_t MaxDAGSize) const noexcept {
    auto Ret = *static_cast<const DerivedConfig *>(this);
    Ret.DAGSizeThreshold = MaxDAGSize;
    return Ret;
  }

  [[nodiscard]] DerivedConfig
  withDAGDepthThreshold(size_t MaxDAGDepth) const noexcept {
    auto Ret = *static_cast<const DerivedConfig *>(this);
    Ret.DAGDepthThreshold = MaxDAGDepth;
    return Ret;
  }

  [[nodiscard]] DerivedConfig
  withNumPathsThreshold(size_t MaxNumPaths) const noexcept {
    auto Ret = *static_cast<const DerivedConfig *>(this);
    Ret.NumPathsThreshold = MaxNumPaths;
    return Ret;
  }

  [[nodiscard]] DerivedConfig withMinimizeDAG(bool DoMinimize) const noexcept {
    auto Ret = *static_cast<const DerivedConfig *>(this);
    Ret.MinimizeDAG = DoMinimize;
    return Ret;
  }
};

struct PathSensitivityConfig
    : PathSensitivityConfigBase<PathSensitivityConfig> {};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYCONFIG_H
