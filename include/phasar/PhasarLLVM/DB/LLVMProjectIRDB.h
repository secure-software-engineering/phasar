/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DB_LLVMPROJECTIRDB_H
#define PHASAR_PHASARLLVM_DB_LLVMPROJECTIRDB_H

#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace psr {
class LLVMProjectIRDB;

template <> struct ProjectIRDBTraits<LLVMProjectIRDB> {
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using m_t = const llvm::Module *;
  using g_t = const llvm::GlobalVariable *;
};

/// \brief Project IR Database that manages a LLVM IR module.
class LLVMProjectIRDB : public ProjectIRDBBase<LLVMProjectIRDB> {
  friend ProjectIRDBBase;

public:
  /// Reads and parses the given LLVM IR file and owns the resulting IR Module.
  /// If an error occurs, an error message is written to stderr and subsequent
  /// calls to isValid() return false.
  explicit LLVMProjectIRDB(const llvm::Twine &IRFileName);

  /// Reads and parses the given LLVM IR file and owns the resulting IR Module.
  /// If an error occurs, an error message is written to stderr and subsequent
  /// calls to isValid() return false.
  [[deprecated("When moving to the next LLVM version, opaque pointers support "
               "is removed completely. Please use one of the other "
               "constructors of LLVMProjectIRDB.")]]
  explicit LLVMProjectIRDB(const llvm::Twine &IRFileName,
                           bool EnableOpaquePointers);

  /// Initializes the new ProjectIRDB with the given IR Module _without_ taking
  /// ownership. The module is optionally being preprocessed.
  ///
  /// CAUTION: Do not manage the same LLVM Module with multiple LLVMProjectIRDB
  /// instances at the same time! This will confuse the ModulesToSlotTracker
  explicit LLVMProjectIRDB(llvm::Module *Mod, bool DoPreprocessing = true);
  /// Initializes the new ProjectIRDB with the given IR Module and takes
  /// ownership of it. The module is optionally being preprocessed.
  explicit LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                           bool DoPreprocessing = true);
  /// Initializes the new ProjectIRDB with the given IR Module and takes
  /// ownership of it. The module is optionally being preprocessed. Takes the
  /// given LLVMContext and binds its lifetime to the lifetime of the
  /// constructed ProjectIRDB
  explicit LLVMProjectIRDB(std::unique_ptr<llvm::Module> Mod,
                           std::unique_ptr<llvm::LLVMContext> Ctx,
                           bool DoPreprocessing = true);

  /// Parses the given LLVM IR file and owns the resulting IR Module.
  /// If an error occurs, an error message is written to stderr and subsequent
  /// calls to isValid() return false.
  explicit LLVMProjectIRDB(llvm::MemoryBufferRef Buf);

  /// Parses the given LLVM IR file and owns the resulting IR Module.
  /// If an error occurs, an error message is written to stderr and subsequent
  /// calls to isValid() return false.
  [[deprecated("When moving to the next LLVM version, opaque pointers support "
               "is removed completely. Please use one of the other "
               "constructors of LLVMProjectIRDB.")]]
  explicit LLVMProjectIRDB(llvm::MemoryBufferRef Buf,
                           bool EnableOpaquePointers);

  LLVMProjectIRDB(const LLVMProjectIRDB &) = delete;
  LLVMProjectIRDB &operator=(const LLVMProjectIRDB &) = delete;

  LLVMProjectIRDB(LLVMProjectIRDB &&) noexcept = default;
  LLVMProjectIRDB &operator=(LLVMProjectIRDB &&) noexcept = default;

  ~LLVMProjectIRDB();

  [[nodiscard]] static llvm::ErrorOr<std::unique_ptr<llvm::Module>>
  getParsedIRModuleOrErr(const llvm::Twine &IRFileName,
                         llvm::LLVMContext &Ctx) noexcept;
  [[nodiscard]] static llvm::ErrorOr<std::unique_ptr<llvm::Module>>
  getParsedIRModuleOrErr(llvm::MemoryBufferRef IRFileContent,
                         llvm::LLVMContext &Ctx) noexcept;

  [[nodiscard]] static llvm::ErrorOr<LLVMProjectIRDB>
  load(const llvm::Twine &IRFileName);

  [[nodiscard]] static LLVMProjectIRDB loadOrExit(const llvm::Twine &IRFileName,
                                                  int ErrorExitCode = 1);
  [[nodiscard]] static LLVMProjectIRDB
  loadOrExit(const llvm::Twine &IRFileName, bool EnableOpaquePointers) = delete;

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
  [[nodiscard]] llvm::Module *getModule() noexcept { return Mod.get(); }

  /// Similar to getInstruction(size_t), but is also able to return global
  /// variables by id
  [[nodiscard]] const llvm::Value *getValueFromId(size_t Id) const noexcept {
    return Id < IdToInst.size() ? IdToInst[Id] : nullptr;
  }

  void emitPreprocessedIR(llvm::raw_ostream &OS) const;

  /// Insert a new function F into the IRDB. F should be present in the same
  /// llvm::Module that is managed by the IRDB. insertFunction should not be
  /// called twice for the same function. Use with care!
  void insertFunction(llvm::Function *F, bool DoPreprocessing = true);

  explicit operator bool() const noexcept { return isValid(); }

private:
  [[nodiscard]] m_t getModuleImpl() const noexcept { return Mod.get(); }
  [[nodiscard]] bool debugInfoAvailableImpl() const;
  [[nodiscard]] FunctionRange getAllFunctionsImpl() const {
    return llvm::map_range(ProjectIRDBBase::getModule()->functions(),
                           Ref2PointerConverter<llvm::Function>{});
  }
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef FunctionName) const {
    return Mod->getFunction(FunctionName);
  }
  [[nodiscard]] f_t
  getFunctionDefinitionImpl(llvm::StringRef FunctionName) const;
  [[nodiscard]] bool
  hasFunctionImpl(llvm::StringRef FunctionName) const noexcept {
    return Mod->getFunction(FunctionName) != nullptr;
  }
  [[nodiscard]] g_t
  getGlobalVariableDefinitionImpl(llvm::StringRef GlobalVariableName) const;
  [[nodiscard]] size_t getNumInstructionsImpl() const noexcept {
    return IdToInst.size() - IdOffset;
  }
  [[nodiscard]] size_t getNumFunctionsImpl() const noexcept {
    return Mod->size();
  }
  [[nodiscard]] size_t getNumGlobalsImpl() const noexcept {
    return Mod->global_size();
  }

  [[nodiscard]] n_t getInstructionImpl(size_t Id) const noexcept {
    // Effectively make use of integer overflow here...
    if (Id - IdOffset < IdToInst.size() - IdOffset) {
      return llvm::cast<llvm::Instruction>(IdToInst[Id]);
    }
    return n_t{};
  }

  [[nodiscard]] auto getAllInstructionsImpl() const noexcept {
    return llvm::map_range(
        llvm::ArrayRef(IdToInst).drop_front(IdOffset),
        [](const llvm::Value *V) { return llvm::cast<llvm::Instruction>(V); });
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

  // LLVMContext is not movable, so wrap it into a unique_ptr
  std::unique_ptr<llvm::LLVMContext> Ctx;
  MaybeUniquePtr<llvm::Module> Mod = nullptr;
  size_t IdOffset = 0;
  llvm::SmallVector<const llvm::Value *, 0> IdToInst;
  llvm::DenseMap<const llvm::Value *, size_t> InstToId;
};

/**
 * Revserses the getMetaDataID function
 */
const llvm::Value *fromMetaDataId(const LLVMProjectIRDB &IRDB,
                                  llvm::StringRef Id);

extern template class ProjectIRDBBase<LLVMProjectIRDB>;
} // namespace psr

#endif // PHASAR_PHASARLLVM_DB_LLVMPROJECTIRDB_H
