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

#include "llvm/Support/Compiler.h"

namespace psr {
template <typename DB> struct ProjectIRDBTraits {
  // using n_t
  // using f_t
  // using m_t
  // using g_t
};

template <typename Derived> class ProjectIRDBBase {
public:
  using n_t = typename ProjectIRDBTraits<Derived>::n_t;
  using f_t = typename ProjectIRDBTraits<Derived>::f_t;
  using m_t = typename ProjectIRDBTraits<Derived>::m_t;
  using g_t = typename ProjectIRDBTraits<Derived>::g_t;

  [[nodiscard]] m_t getModule() const noexcept {
    return self().getModuleImpl();
  }

  [[nodiscard]] bool debugInfoAvailable() const {
    return self().debugInfoAvailableImpl();
  }

  [[nodiscard]] decltype(auto) getAllFunctions() const {
    static_assert(
        is_iterable_over_v<decltype(self().getAllFunctionsImpl()), f_t>);
    return self().getAllFunctionsImpl();
  }

  [[nodiscard]] f_t getFunction(llvm::StringRef FunctionName) const {
    return self().getFunctionImpl(FunctionName);
  }
  [[nodiscard]] f_t getFunctionDefinition(llvm::StringRef FunctionName) const {
    return self().getFunctionDefinitionImpl(FunctionName);
  }

  [[nodiscard]] g_t
  getGlobalVariableDefinition(llvm::StringRef GlobalVariableName) const {
    return self().getGlobalVariableDefinitionImpl(GlobalVariableName);
  }

  [[nodiscard]] size_t getNumInstructions() const noexcept {
    return self().getNumInstructionsImpl();
  }
  [[nodiscard]] size_t getNumGlobals() const noexcept {
    return self().getNumGlobalsImpl();
  }
  [[nodiscard]] size_t getNumFunctions() const noexcept {
    return self().getNumFunctionsImpl();
  }

  [[nodiscard]] n_t getInstruction(size_t Id) const {
    return self().getInstructionImpl(Id);
  }
  [[nodiscard]] size_t getInstructionId(n_t Inst) const {
    return self().getInstructionId(Inst);
  }

  [[nodiscard]] bool isValid() const noexcept { return self().isValidImpl(); }

  LLVM_DUMP_METHOD void dump() const { self().dumpImpl(); }

  ProjectIRDBBase(const ProjectIRDBBase &) = delete;
  ProjectIRDBBase &operator=(const ProjectIRDBBase &) = delete;

  ProjectIRDBBase(ProjectIRDBBase &&) noexcept = default;
  ProjectIRDBBase &operator=(ProjectIRDBBase &&) noexcept = default;

private:
  friend Derived;
  ProjectIRDBBase() noexcept = default;

  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};
} // namespace psr

#endif // PHASAR_DB_PROJECTIRDBBASE_H
