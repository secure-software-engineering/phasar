/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DB_LLVMPROJECTIRDB_H
#define PHASAR_DB_LLVMPROJECTIRDB_H

#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>

namespace psr {
class LLVMProjectIRDB;

template <> struct ProjectIRDBTraits<LLVMProjectIRDB> {
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using m_t = const llvm::Module *;
  using g_t = const llvm::GlobalVariable *;
};

class LLVMProjectIRDB : public ProjectIRDBBase<LLVMProjectIRDB> {
  friend ProjectIRDBBase;

public:
  /// Reads and parses the  given LLVM IR file and owns the resolting IR Module
  explicit LLVMProjectIRDB(llvm::StringRef IRFileName);
  /// Initializes the new ProjectIRDB with the given IR Module _without_ taking
  /// ownership. The module is not being preprocessed
  explicit LLVMProjectIRDB(llvm::Module *Mod);
  /// Initializes the new ProjectIRDB with the given IR Moduleand takes
  /// ownership of it
  explicit LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                           bool DoPreprocessing = true);

  /// Also use the const overload
  using ProjectIRDBBase::getFunction;
  /// Non-const overload
  [[nodiscard]] llvm::Function *getFunction(llvm::StringRef FunctionName) {
    return Mod->getFunction(FunctionName);
  }

  /// Also use the const overload
  using ProjectIRDBBase::getFunctionDefinition;
  /// Non-const overload
  [[nodiscard]] llvm::Function *
  getFunctionDefinition(llvm::StringRef FunctionName);

  /// Also use the const overload
  using ProjectIRDBBase::getModule;
  /// Non-const overload
  [[nodiscard]] llvm::Module *getModule() { return Mod.get(); }

private:
  [[nodiscard]] m_t getModuleImpl() const noexcept { return Mod.get(); }
  [[nodiscard]] bool debugInfoAvailableImpl() const;
  [[nodiscard]] auto getAllFunctionsImpl() const {
    return llvm::map_range(Mod->functions(),
                           [](const llvm::Function &F) { return &F; });
  }
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef FunctionName) const {
    return Mod->getFunction(FunctionName);
  }
  [[nodiscard]] f_t
  getFunctionDefinitionImpl(llvm::StringRef FunctionName) const;
  [[nodiscard]] g_t
  getGlobalVariableDefinitionImpl(llvm::StringRef GlobalVariableName) const;
  [[nodiscard]] size_t getNumInstructionsImpl() const noexcept {
    return IdToInst.size();
  }
  [[nodiscard]] size_t getNumFunctionsImpl() const noexcept {
    return Mod->size();
  }
  [[nodiscard]] size_t getNumGlobalsImpl() const noexcept {
    return Mod->global_size();
  }

  [[nodiscard]] n_t getInstructionImpl(size_t Id) const noexcept {
    assert(Id < IdToInst.size());
    return IdToInst[Id];
  }
  [[nodiscard]] size_t getInstructionIdImpl(n_t Inst) const {
    auto It = InstToId.find(Inst);
    assert(It != InstToId.end());
    return It->second;
  }
  [[nodiscard]] bool isValidImpl() const noexcept;

  void dumpImpl() const;

  void initInstructionIds();
  /// XXX Later we might get rid of the metadata IDs entirely and therefore of
  /// the preprocessing as well
  void preprocessModule(llvm::Module *NonConstMod);

  llvm::LLVMContext Ctx;
  MaybeUniquePtr<llvm::Module> Mod = nullptr;
  size_t IdOffset = 0;
  llvm::SmallVector<const llvm::Instruction *, 0> IdToInst;
  llvm::DenseMap<const llvm::Instruction *, size_t> InstToId;
};
} // namespace psr

#endif // PHASAR_DB_LLVMPROJECTIRDB_H
