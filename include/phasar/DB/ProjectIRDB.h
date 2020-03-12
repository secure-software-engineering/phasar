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

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

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
  bool wasCompiledWithDebugInfo(llvm::Module *M) const;

  void preprocessAllModules();

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

  bool containsSourceFile(const std::string &File) const;

  bool empty() const;

  bool debugInfoAvailable() const;

  llvm::Module *getModule(const std::string &ModuleName);

  inline std::set<llvm::Module *> getAllModules() const {
    std::set<llvm::Module *> ModuleSet;
    for (auto &[File, Module] : Modules) {
      ModuleSet.insert(Module.get());
    }
    return ModuleSet;
  }

  std::set<const llvm::Function *> getAllFunctions() const;

  const llvm::Function *
  getFunctionDefinition(const std::string &FunctionName) const;

  const llvm::Function *getFunction(const std::string &FunctionName) const;

  const llvm::GlobalVariable *
  getGlobalVariableDefinition(const std::string &GlobalVariableName) const;

  llvm::Module *getModuleDefiningFunction(const std::string &FunctionName);

  const llvm::Module *
  getModuleDefiningFunction(const std::string &FunctionName) const;

  std::set<const llvm::Instruction *> getAllocaInstructions() const;

  /**
   * LLVM's intrinsic global variables are excluded.
   *
   * @brief Returns all stack and heap allocations, including global variables.
   */
  std::set<const llvm::Value *> getAllMemoryLocations() const;

  std::set<std::string> getAllSourceFiles() const;

  std::set<const llvm::Type *> getAllocatedTypes() const;

  std::set<const llvm::StructType *> getAllocatedStructTypes() const;

  std::set<const llvm::Instruction *> getRetOrResInstructions() const;

  std::size_t getNumberOfModules() const;

  llvm::Instruction *getInstruction(std::size_t id);

  std::size_t getInstructionID(const llvm::Instruction *I) const;

  void print() const;

  void emitPreprocessedIR(std::ostream &os = std::cout,
                          bool shortendIR = true) const;

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
  std::string valueToPersistedString(const llvm::Value *V);
  /**
   * @brief Convertes the given string back into the llvm::Value it represents.
   * @return Pointer to the converted llvm::Value.
   */
  const llvm::Value *persistedStringToValue(const std::string &StringRep);
};

} // namespace psr

#endif
