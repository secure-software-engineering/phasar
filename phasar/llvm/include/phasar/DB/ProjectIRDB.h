/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DB_PROJECTIRDB_H_
#define PHASAR_DB_PROJECTIRDB_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

#include "phasar/Utils/EnumFlags.h"

namespace llvm {
class Value;
class Instruction;
class Type;
class Function;
class GlobalVariable;
} // namespace llvm

namespace psr {

enum class IRDBOptions : uint32_t { NONE = 0, WPA = (1 << 0), OWNS = (1 << 1) };

/**
 * This class owns the LLVM IR code of the project under analysis and some
 * very important information associated with the IR.
 * When an object of this class is destroyed it will clean up all IR related
 * stuff that is stored in it.
 */
class ProjectIRDB {
private:
  llvm::Module *WPAModule = nullptr;
  IRDBOptions Options;
  llvm::PassBuilder PB;
  llvm::ModuleAnalysisManager MAM;
  llvm::ModulePassManager MPM;
  // Stores all allocation instructions
  std::set<const llvm::Instruction *> AllocaInstructions;
  // Stores all allocated types
  std::set<const llvm::Type *> AllocatedTypes;
  // Return or resum instructions
  std::set<const llvm::Instruction *> RetOrResInstructions;
  // Stores the contexts
  std::vector<std::unique_ptr<llvm::LLVMContext>> Contexts;
  // Contains all modules that correspond to a project and owns them
  std::map<std::string, std::unique_ptr<llvm::Module>> Modules;
  // Maps an id to its corresponding instruction
  std::map<std::size_t, llvm::Instruction *> IDInstructionMapping;

  void buildIDModuleMapping(llvm::Module *M);

  void preprocessModule(llvm::Module *M);
  static bool wasCompiledWithDebugInfo(llvm::Module *M) {
    return M->getNamedMetadata("llvm.dbg.cu") != nullptr;
  };

  void preprocessAllModules();

  [[nodiscard]] llvm::Function *
  internalGetFunction(llvm::StringRef FunctionName) const;
  [[nodiscard]] llvm::Function *
  internalGetFunctionDefinition(llvm::StringRef FunctionName) const;

public:
  /// Constructs an empty ProjectIRDB
  ProjectIRDB(IRDBOptions Options);
  /// Constructs a ProjectIRDB from a bunch of LLVM IR files
  ProjectIRDB(const std::vector<std::string> &IRFiles,
              IRDBOptions Options = (IRDBOptions::WPA | IRDBOptions::OWNS));
  /// Constructs a ProjecIRDB from a bunch of LLVM Modules
  ProjectIRDB(const std::vector<llvm::Module *> &Modules,
              IRDBOptions Options = IRDBOptions::WPA);

  ProjectIRDB(ProjectIRDB &&) = default;
  ProjectIRDB &operator=(ProjectIRDB &&) = default;

  ProjectIRDB(ProjectIRDB &) = delete;
  ProjectIRDB &operator=(const ProjectIRDB &) = delete;

  ~ProjectIRDB();

  void insertModule(llvm::Module *M);

  // add WPA support by providing a fat completely linked module
  void linkForWPA();
  // get a completely linked module for the WPA_MODE
  llvm::Module *getWPAModule();

  [[nodiscard]] inline bool containsSourceFile(const std::string &File) const {
    return Modules.find(File) != Modules.end();
  };

  [[nodiscard]] inline bool empty() const { return Modules.empty(); };

  [[nodiscard]] bool debugInfoAvailable() const;

  llvm::Module *getModule(const std::string &ModuleName);

  [[nodiscard]] inline std::set<llvm::Module *> getAllModules() const {
    std::set<llvm::Module *> ModuleSet;
    for (const auto &[File, Module] : Modules) {
      ModuleSet.insert(Module.get());
    }
    return ModuleSet;
  }

  [[nodiscard]] std::set<const llvm::Function *> getAllFunctions() const;

  [[nodiscard]] const llvm::Function *
  getFunctionDefinition(llvm::StringRef FunctionName) const;
  [[nodiscard]] llvm::Function *
  getFunctionDefinition(llvm::StringRef FunctionName);

  [[nodiscard]] const llvm::Function *
  getFunction(llvm::StringRef FunctionName) const;
  [[nodiscard]] llvm::Function *getFunction(llvm::StringRef FunctionName);

  [[nodiscard]] const llvm::GlobalVariable *
  getGlobalVariableDefinition(const std::string &GlobalVariableName) const;

  llvm::Module *getModuleDefiningFunction(const std::string &FunctionName);

  [[nodiscard]] const llvm::Module *
  getModuleDefiningFunction(const std::string &FunctionName) const;

  [[nodiscard]] inline const std::set<const llvm::Instruction *> &
  getAllocaInstructions() const {
    return AllocaInstructions;
  };

  /**
   * LLVM's intrinsic global variables are excluded.
   *
   * @brief Returns all stack and heap allocations, including global variables.
   */
  [[nodiscard]] std::set<const llvm::Value *> getAllMemoryLocations() const;

  [[nodiscard]] std::set<std::string> getAllSourceFiles() const;

  [[nodiscard]] std::set<const llvm::Type *> getAllocatedTypes() const {
    return AllocatedTypes;
  };

  [[nodiscard]] std::set<const llvm::StructType *>
  getAllocatedStructTypes() const;

  [[nodiscard]] inline std::set<const llvm::Instruction *>
  getRetOrResInstructions() const {
    return RetOrResInstructions;
  };

  [[nodiscard]] inline std::size_t getNumberOfModules() const {
    return Modules.size();
  };

  [[nodiscard]] inline std::size_t getNumInstructions() const {
    return IDInstructionMapping.size();
  }

  [[nodiscard]] std::size_t getNumGlobals() const;

  [[nodiscard]] llvm::Instruction *getInstruction(std::size_t Id) const;

  [[nodiscard]] static std::size_t getInstructionID(const llvm::Instruction *I);

  void print() const;

  void emitPreprocessedIR(llvm::raw_ostream &OS = llvm::outs(),
                          bool ShortenIR = false) const;

  /**
   * Allows the (de-)serialization of Instructions, Arguments, GlobalValues and
   * Operands into unique Hexastore string representation.
   *
   * What values can be serialized and what scheme is used?
   *
   * 	1. Instructions
   *
   * 		<function name>.<id>
   *
   * 	2. Formal parameters
   *
   *		<function name>.f<arg-no>
   *
   *	3. Global variables
   *
   *		<global variable name>
   *
   *	4. ZeroValue
   *
   *		<ZeroValueInternalName>
   *
   *	5. Operand of an instruction
   *
   *		<function name>.<id>.o.<operand no>
   *
   * @brief Creates a unique string representation for any given
   * llvm::Value.
   */
  [[nodiscard]] static std::string valueToPersistedString(const llvm::Value *V);
  /**
   * @brief Convertes the given string back into the llvm::Value it represents.
   * @return Pointer to the converted llvm::Value.
   */
  [[nodiscard]] const llvm::Value *
  persistedStringToValue(const std::string &StringRep) const;
};

} // namespace psr

#endif
