/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/
#pragma once

#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/StringRef.h"

#include <concepts>

namespace psr {

template <typename T>
concept ProjectSymbolTable = requires(const T &ST, llvm::StringRef Name,
                                      size_t InstId, typename T::n_t Inst) {
  /// Reference to a Instruction/Statement
  typename T::n_t;
  /// Reference to a function
  typename T::f_t;
  /// Reference to a program module (potentially containing the whole
  /// program)
  typename T::m_t;
  /// Reference to a Global Variable
  typename T::g_t;

  /// Returns the function (declaration or definition) if available,
  /// nullptr/nullopt otherwise.
  { ST.getFunction(Name) } -> std::convertible_to<Nullable<typename T::f_t>>;

  /// Returns the function's definition if available, null otherwise.
  {
    ST.getFunctionDefinition(Name)
  } -> std::convertible_to<Nullable<typename T::f_t>>;

  /// Returns whether the IRDB contains a function (declaration or
  /// definition) with the given name.
  { ST.hasFunction(Name) } -> std::convertible_to<bool>;

  /// Returns the global variable (declaration or definition) if available,
  /// nullptr/nullopt otherwise.
  {
    ST.getGlobalVariable(Name)
  } -> std::convertible_to<Nullable<typename T::g_t>>;

  /// Returns the global variable's definition if available, nullptr/nullopt
  /// otherwise.
  {
    ST.getGlobalVariableDefinition(Name)
  } -> std::convertible_to<Nullable<typename T::g_t>>;

  /// Returns the instruction to the corresponding Id. Returns
  /// nullptr/nullopt, if there is no instruction for this Id
  {
    ST.getInstruction(InstId)
  } -> std::convertible_to<Nullable<typename T::n_t>>;

  /// Returns an instruction's Id. The instruction must belong to the managed
  /// module for this function to work.
  ///
  /// You can expect the instruction-ids to be sequential in the interval [0,
  /// N), where N is the total number of managed instructions.
  { ST.getInstructionId(Inst) } -> std::convertible_to<size_t>;
};

/// This concept describes the minimum requirements for a project-IR-database.
/// Each phasar-analysis works on a program that is managed by a ProjectIRDB.
template <typename T>
concept ProjectIRDB =
    requires(const T &DB, typename T::n_t Inst, typename T::f_t Fun) {
      /// Returns the managed module
      { DB.getModule() } noexcept -> std::convertible_to<typename T::m_t>;

      /// Check if debug information are available.
      { DB.debugInfoAvailable() } -> std::convertible_to<bool>;

      /// Returns a range of all function definitions and declarations available
      { DB.getAllFunctions() } -> psr::is_iterable_over_v<typename T::f_t>;

      /// Returns the number of functions in the managed module.
      { DB.getNumFunctions() } -> std::convertible_to<size_t>;

      /// Returns a range of all global variables (and global constants, e.g,
      /// string literals) in the managed module
      { DB.getAllGlobals() } -> psr::is_iterable_over_v<typename T::g_t>;

      /// Returns the number of global variables (and global constants) in the
      /// managed module.
      { DB.getNumGlobals() } -> std::convertible_to<size_t>;

      /// Returns a range of all instructions available.
      { DB.getAllInstructions() } -> psr::is_iterable_over_v<typename T::n_t>;

      /// Returns the number of instruction in the managed module.
      { DB.getNumInstructions() } -> std::convertible_to<size_t>;

      /// Returns the function that contains the given instruction Inst.
      { DB.getFunctionOf(Inst) } -> std::convertible_to<typename T::f_t>;

      /// Returns an iterable range of all instructions of the given function
      /// that are part of the control-flow graph.
      {
        DB.getAllInstructionsOf(Fun)
      } -> psr::is_iterable_over_v<typename T::n_t>;

      requires ProjectSymbolTable<T>;
    };

// NOLINTNEXTLINE(readability-identifier-naming)
auto IRDBGetFunctionDef(const ProjectIRDB auto *IRDB) noexcept {
  return [IRDB](llvm::StringRef Name) {
    return IRDB->getFunctionDefinition(Name);
  };
}
} // namespace psr
