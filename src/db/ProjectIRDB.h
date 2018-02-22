/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ANALYSIS_ProjectIRDB_H_
#define ANALYSIS_ProjectIRDB_H_

#include "../analysis/ifds_ide/ZeroValue.h"
#include "../analysis/passes/GeneralStatisticsPass.h"
#include "../analysis/passes/ValueAnnotationPass.h"
#include "../analysis/points-to/PointsToGraph.h"
#include "../lib/LLVMShorthands.h"
#include "../utils/EnumFlags.h"
#include "../utils/PAMM.h"
#include "../utils/utils.h"
#include <algorithm>
#include <cassert>
#include <clang/Basic/Diagnostic.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

enum class IRDBOptions : uint32_t {
  NONE = 0,
  MEM2REG = (1 << 0),
  WPA = (1 << 1)
};

/**
 * This class owns the LLVM IR code of the project under analysis and some
 * very important information associated with the IR.
 * When an object of this class is destroyed it will clean up all IR related
 * stuff that is stored in it.
 */
class ProjectIRDB {
private:
  llvm::Module *WPAMOD = nullptr;
  IRDBOptions Options;
  void compileAndAddToDB(std::vector<const char *> CompileCommand);
  std::vector<std::string> header_search_paths;
  static const std::set<std::string> unknown_flags;
  void setupHeaderSearchPaths();
  // Stores all source files that have been examined
  std::set<std::string> source_files;
  // Contains all contexts for all modules and owns them
  std::map<std::string, std::unique_ptr<llvm::LLVMContext>> contexts;
  // Contains all modules that correspond to a project and owns them
  std::map<std::string, std::unique_ptr<llvm::Module>> modules;
  // Maps function names to the module they are !defined! in
  std::map<std::string, std::string> functions;
  // Maps globals to the module they are !defined! in
  std::map<std::string, std::string> globals;
  // Maps an id to its corresponding instruction
  std::map<std::size_t, llvm::Instruction *> instructions;
  // Maps a function to its points-to graph
  std::map<std::string, std::unique_ptr<PointsToGraph>> ptgs;

  void buildFunctionModuleMapping(llvm::Module *M);
  void buildGlobalModuleMapping(llvm::Module *M);
  void buildIDModuleMapping(llvm::Module *M);
  void preprocessModule(llvm::Module *M);

public:
  /// Constructs an empty ProjectIRDB
  ProjectIRDB(enum IRDBOptions Opt);
  /// Constructs a ProjectIRDB from a bunch of llvm IR files
  ProjectIRDB(const std::vector<std::string> &IRFiles,
              enum IRDBOptions Opt = IRDBOptions::NONE);
  /// Constructs a ProjectIRDB from a CompilationDatabase (only for simple
  /// projects)
  ProjectIRDB(const clang::tooling::CompilationDatabase &CompileDB,
              enum IRDBOptions Opt);
  /// Constructs a ProjectIRDB from files wich may have to be compiled to llvm
  /// IR
  ProjectIRDB(const std::vector<std::string> &Files,
              std::vector<const char *> CompileArgs, enum IRDBOptions Opt);
  ProjectIRDB(ProjectIRDB &&) = default;
  ~ProjectIRDB() = default;

  void preprocessIR();

  // add WPA support by providing a fat completely linked module
  void linkForWPA();
  // get a completely linked module for the WPA_MODE
  llvm::Module *getWPAModule();
  bool containsSourceFile(const std::string &src);
  bool empty();
  llvm::LLVMContext *getLLVMContext(const std::string &ModuleName);
  void insertModule(std::unique_ptr<llvm::Module> M);
  llvm::Module *getModule(const std::string &ModuleName);
  std::set<llvm::Module *> getAllModules() const;
  std::set<const llvm::Function *> getAllFunctions();
  std::set<std::string> getAllSourceFiles();
  std::size_t getNumberOfModules();
  llvm::Module *getModuleDefiningFunction(const std::string &FunctionName);
  llvm::Function *getFunction(const std::string &FunctionName);
  llvm::GlobalVariable *
  getGlobalVariable(const std::string &GlobalVariableName);
  llvm::Instruction *getInstruction(std::size_t id);
  std::size_t getInstructionID(const llvm::Instruction *I);
  PointsToGraph *getPointsToGraph(const std::string &FunctionName);
  void insertPointsToGraph(const std::string &FunctionName, PointsToGraph *ptg);
  void print();
  void exportPATBCJSON();
  std::string valueToPersistedString(const llvm::Value *V);
  const llvm::Value *persistedStringToValue(const std::string &StringRep);
};

#endif /* ANALYSIS_ProjectIRDB_HH_ */
