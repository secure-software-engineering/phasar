/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DB_PROJECTIRDBBASE_H
#define PHASAR_DB_PROJECTIRDBBASE_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

namespace psr {
template <typename DB> struct ProjectIRDBTraits {
  // using n_t
  // using f_t
  // using m_t
  // using g_t
};

/// This class owns the IR code of the project under analysis and some
/// very important information associated with the IR.
/// When an object of this class is destroyed it will clean up all IR related
/// stuff that is stored in it.
///
/// m_t - module type
/// f_t - function type
/// n_t - instruction type
/// g_t - global variable type
///
/// \remark XXX Once we have upgraded to C++20, we might want to use a concept
/// instead...
template <typename Derived> class ProjectIRDBBase {
public:
  using n_t = typename ProjectIRDBTraits<Derived>::n_t;
  using f_t = typename ProjectIRDBTraits<Derived>::f_t;
  using m_t = typename ProjectIRDBTraits<Derived>::m_t;
  using g_t = typename ProjectIRDBTraits<Derived>::g_t;

  /// Returns the managed module
  [[nodiscard]] m_t getModule() const noexcept {
    assert(isValid());
    return self().getModuleImpl();
  }

  /// Check if debug information are available.
  [[nodiscard]] bool debugInfoAvailable() const {
    assert(isValid());
    return self().debugInfoAvailableImpl();
  }

  /// Returns a range of all function definitions and declarations available
  [[nodiscard]] decltype(auto) getAllFunctions() const {
    static_assert(
        is_iterable_over_v<decltype(self().getAllFunctionsImpl()), f_t>);
    assert(isValid());
    return self().getAllFunctionsImpl();
  }

  // Returns the function's definition if available, its declaration otherwise.
  [[nodiscard]] f_t getFunction(llvm::StringRef FunctionName) const {
    assert(isValid());
    return self().getFunctionImpl(FunctionName);
  }
  /// Returns the function's definition if available, null otherwise.
  [[nodiscard]] f_t getFunctionDefinition(llvm::StringRef FunctionName) const {
    assert(isValid());
    return self().getFunctionDefinitionImpl(FunctionName);
  }
  /// Returns whether the IRDB contains a function with the given name.
  [[nodiscard]] bool hasFunction(llvm::StringRef FunctionName) const noexcept {
    assert(isValid());
    return self().hasFunctionImpl(FunctionName);
  }

  /// Returns the global variable's definition if available, null otherwise.
  [[nodiscard]] g_t
  getGlobalVariableDefinition(llvm::StringRef GlobalVariableName) const {
    assert(isValid());
    return self().getGlobalVariableDefinitionImpl(GlobalVariableName);
  }

  /// Returns the number of instruction in the managed module.
  [[nodiscard]] size_t getNumInstructions() const noexcept {
    assert(isValid());
    return self().getNumInstructionsImpl();
  }
  /// Returns the number of global variables in the managed module.
  [[nodiscard]] size_t getNumGlobals() const noexcept {
    assert(isValid());
    return self().getNumGlobalsImpl();
  }
  /// Returns the number of functions in the managed module.
  [[nodiscard]] size_t getNumFunctions() const noexcept {
    assert(isValid());
    return self().getNumFunctionsImpl();
  }

  /// Returns the instruction to the corresponding Id. Returns nullptr, if there
  /// is no instruction for this Id
  [[nodiscard]] n_t getInstruction(size_t Id) const {
    assert(isValid());
    return self().getInstructionImpl(Id);
  }
  /// Returns an instruction's ID. The instruction must belong to the managed
  /// module for this function to work
  [[nodiscard]] size_t getInstructionId(n_t Inst) const {
    assert(isValid());
    return self().getInstructionIdImpl(Inst);
  }

  [[nodiscard]] decltype(auto) getAllInstructions() const {
    static_assert(
        is_iterable_over_v<decltype(self().getAllInstructionsImpl()), n_t>);
    assert(isValid());
    return self().getAllInstructionsImpl();
  }

  /// Sanity check to verify that th IRDB really manages a Module and all
  /// functions work properly.
  /// All functions of this IRDB require isValid() to return true, if not stated
  /// differently -- otherwise, they must not be called.
  /// This function (obviously) does not require isValid() to return true.
  [[nodiscard]] bool isValid() const noexcept { return self().isValidImpl(); }

  /// Dumps the managed module to llvm::dbgs() if isValid(); otherwise prints
  /// "<Invalid Module>".
  /// This function can be invoked from a debugger.
  LLVM_DUMP_METHOD void dump() const {
    if (!isValid()) {
      llvm::dbgs() << "<Invalid Module>\n";
      llvm::dbgs().flush();
      return;
    }
    self().dumpImpl();
  }

  ProjectIRDBBase(const ProjectIRDBBase &) = delete;
  ProjectIRDBBase &operator=(const ProjectIRDBBase &) = delete;

  ProjectIRDBBase(ProjectIRDBBase &&) noexcept = default;
  ProjectIRDBBase &operator=(ProjectIRDBBase &&) noexcept = default;

  ~ProjectIRDBBase() = default;

private:
  friend Derived;
  ProjectIRDBBase() noexcept = default;

  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

template <typename DB>
// NOLINTNEXTLINE(readability-identifier-naming)
auto IRDBGetFunctionDef(const ProjectIRDBBase<DB> *IRDB) noexcept {
  return [IRDB](llvm::StringRef Name) {
    return IRDB->getFunctionDefinition(Name);
  };
}

} // namespace psr

#endif // PHASAR_DB_PROJECTIRDBBASE_H
