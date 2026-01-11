#pragma once

#include "phasar/DB/ProjectIRDB.h"

namespace psr {
/// Describes useful properties of the program intermediate representation (IR).
template <typename T>
concept IRDomain = requires() {
  /// (Control-flow) Node --- Specifies the type of a node in the
  /// (inter-procedural) control-flow graph and can be though of as an
  /// individual statement or instruction of the target program.
  typename T::n_t;
  /// Function --- Specifies the type of functions/procedures in the target
  /// program.
  typename T::f_t;
  /// (Pointer) value --- Specifies the type of pointers.
  typename T::v_t;
  /// The ProjectIRDB type --- Provides access to the IR.
  typename T::db_t;
  requires ProjectIRDB<typename T::db_t>;
};
} // namespace psr
