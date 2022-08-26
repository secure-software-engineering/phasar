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
struct PathSensitivityConfig {
  size_t DAGSizeThreshold = SIZE_MAX;
  size_t DAGDepthThreshold = SIZE_MAX;
  size_t NumPathsThreshold = SIZE_MAX;
  bool MinimizeDAG = true;

  [[nodiscard]] PathSensitivityConfig
  withDAGSizeThreshold(size_t MaxDAGSize) const noexcept {
    auto Ret = *this;
    Ret.DAGSizeThreshold = MaxDAGSize;
    return Ret;
  }

  [[nodiscard]] PathSensitivityConfig
  withDAGDepthThreshold(size_t MaxDAGDepth) const noexcept {
    auto Ret = *this;
    Ret.DAGDepthThreshold = MaxDAGDepth;
    return Ret;
  }

  [[nodiscard]] PathSensitivityConfig
  withNumPathsThreshold(size_t MaxNumPaths) const noexcept {
    auto Ret = *this;
    Ret.NumPathsThreshold = MaxNumPaths;
    return Ret;
  }

  [[nodiscard]] PathSensitivityConfig
  withMinimizeDAG(bool DoMinimize) const noexcept {
    auto Ret = *this;
    Ret.MinimizeDAG = DoMinimize;
    return Ret;
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYCONFIG_H
