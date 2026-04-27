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

#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace psr {

/// \brief Project IR Database that manages a LLVM IR module.
class LLVMProjectIRDB {

public:
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using m_t = const llvm::Module *;
  using g_t = const llvm::GlobalVariable *;

  /// Reads and parses the given LLVM IR file and owns the resulting IR Module.
  /// If an error occurs, an error message is written to stderr and subsequent
  /// calls to isValid() return false.
  explicit LLVMProjectIRDB(const llvm::Twine &IRFileName);

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

  /// Non-const overload
  [[nodiscard]] llvm::Function *getFunction(llvm::StringRef FunctionName) {
    assert(isValid());
    return Mod->getFunction(FunctionName);
  }
  /// Returns the function if available, nullptr/nullopt otherwise.
  [[nodiscard]] f_t getFunction(llvm::StringRef FunctionName) const {
    assert(isValid());
    return Mod->getFunction(FunctionName);
  }

  /// Returns a mutable pointer to the function's definition if available, null
  /// otherwise.
  [[nodiscard]] llvm::Function *
  getFunctionDefinition(llvm::StringRef FunctionName);

  /// Returns the function's definition if available, null otherwise.
  [[nodiscard]] f_t getFunctionDefinition(llvm::StringRef FunctionName) const;

  /// Returns the managed module
  [[nodiscard]] llvm::Module *getModule() noexcept { return Mod.get(); }

  /// Returns the managed module
  [[nodiscard]] const llvm::Module *getModule() const noexcept {
    return Mod.get();
  }

  /// Similar to getInstruction(size_t), but is also able to return global
  /// variables by id
  [[nodiscard]] const llvm::Value *getValueFromId(size_t Id) const noexcept {
    assert(isValid());
    return Id < IdToInst.size() ? IdToInst[Id] : nullptr;
  }

  void emitPreprocessedIR(llvm::raw_ostream &OS) const;

  /// Insert a new function F into the IRDB. F should be present in the same
  /// llvm::Module that is managed by the IRDB. insertFunction should not be
  /// called twice for the same function. Use with care!
  void insertFunction(llvm::Function *F, bool DoPreprocessing = true);

  /// Returns the function that contains the given instruction Inst.
  ///
  /// \remark This function should eventually replace
  /// LLVMBasedCFG::getFunctionOf()
  [[nodiscard]] f_t getFunctionOf(n_t Inst) const {
    return Inst ? Inst->getFunction() : nullptr;
  }

  /// Returns an iterable range of all instructions of the given function that
  /// are part of the control-flow graph.
  ///
  /// \remark This function should eventually replace
  /// LLVMBasedCFG::getAllInstructionsOf()
  [[nodiscard]] auto getAllInstructionsOf(f_t Fun) const {
    return llvm::map_range(llvm::instructions(Fun),
                           Ref2PointerConverter<llvm::Instruction>{});
  }

  /// Returns a range of all global variables (and global constants, e.g, string
  /// literals) in the managed module
  [[nodiscard]] auto getAllGlobals() const {
    assert(isValid());
    return llvm::map_range(std::as_const(*Mod).globals(),
                           Ref2PointerConverter<llvm::GlobalVariable>{});
  }

  explicit operator bool() const noexcept { return isValid(); }

  [[nodiscard]] bool debugInfoAvailable() const;
  [[nodiscard]] FunctionRange getAllFunctions() const {
    return llvm::map_range(getModule()->functions(),
                           Ref2PointerConverter<llvm::Function>{});
  }

  [[nodiscard]] bool hasFunction(llvm::StringRef FunctionName) const noexcept {
    assert(isValid());
    return Mod->getFunction(FunctionName) != nullptr;
  }

  [[nodiscard]] g_t getGlobalVariable(llvm::StringRef GlobalVariableName) const;
  [[nodiscard]] g_t
  getGlobalVariableDefinition(llvm::StringRef GlobalVariableName) const;

  [[nodiscard]] size_t getNumInstructions() const noexcept {
    return IdToInst.size() - IdOffset;
  }
  [[nodiscard]] size_t getNumFunctions() const noexcept {
    assert(isValid());
    return Mod->size();
  }
  [[nodiscard]] size_t getNumGlobals() const noexcept {
    assert(isValid());
    return Mod->global_size();
  }

  [[nodiscard]] n_t getInstruction(size_t Id) const noexcept {
    // Effectively make use of integer overflow here...
    if (Id - IdOffset < IdToInst.size() - IdOffset) {
      return llvm::cast<llvm::Instruction>(IdToInst[Id]);
    }
    return n_t{};
  }

  [[nodiscard]] auto getAllInstructions() const noexcept {
    return llvm::map_range(
        llvm::ArrayRef(IdToInst).drop_front(IdOffset),
        [](const llvm::Value *V) { return llvm::cast<llvm::Instruction>(V); });
  }

  [[nodiscard]] size_t getInstructionId(n_t Inst) const {
    auto It = InstToId.find(Inst);
    assert(It != InstToId.end());
    return It->second;
  }
  [[nodiscard]] bool isValid() const noexcept;

  void dump() const;

private:
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

} // namespace psr

#endif // PHASAR_PHASARLLVM_DB_LLVMPROJECTIRDB_H
