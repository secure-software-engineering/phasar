#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <cstdint>

namespace psr {

/// The direction in which the DFISolver's reachability index is built. Both
/// directions answer the same queries; they differ in which enumeration is
/// cheap and in the index size.
enum class DFIIndexDirection : uint8_t {
  Forward,
  Backward,
};

/// Static configuration of the DFISolver. Derive from it to change options.
struct DFISolverConfig {
  /// Propagate the zero fact along all flow functions, even if the analysis
  /// problem does not do so explicitly
  static constexpr bool AutoAddZero = true;
  /// Build the reachability index required by DFISolver::getReachability()
  static constexpr bool BuildQueryIndex = true;
  static constexpr DFIIndexDirection IndexDirection =
      DFIIndexDirection::Forward;
};

/// DFISolverConfig without reachability index; use it if only the IFDS
/// results are of interest
struct DFISolverConfigNoIndex : DFISolverConfig {
  static constexpr bool BuildQueryIndex = false;
};

/// DFISolverConfig with a backward reachability index; use it if mostly the
/// origins of facts are queried
struct DFISolverConfigBackward : DFISolverConfig {
  static constexpr DFIIndexDirection IndexDirection =
      DFIIndexDirection::Backward;
};

} // namespace psr
