#ifndef ANALYSIS_ProjectIRDB_H_
#define ANALYSIS_ProjectIRDB_H_

#include "../analysis/ifds_ide/ZeroValue.h"
#include "../analysis/passes/GeneralStatisticsPass.h"
#include "../analysis/passes/ValueAnnotationPass.h"
#include "../analysis/points-to/PointsToGraph.h"
#include "../lib/LLVMShorthands.h"
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
#include <iostream>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
using namespace std;

/**
 * This class owns the LLVM IR code of the project under analysis and some
 * very important information associated with the IR.
 * When an object of this class is destroyed it will clean up all IR related
 * stuff that is stored in it.
 */
class ProjectIRDB {
private:
  llvm::Module *WPAMOD = nullptr;
  void compileAndAddToDB(vector<const char *> CompileCommand);
  vector<string> header_search_paths;
  static const set<string> unknown_flags;
  void setupHeaderSearchPaths();
  // Stores all source files that have been examined
  set<string> source_files;
  // Contains all contexts for all modules and owns them
  map<string, unique_ptr<llvm::LLVMContext>> contexts;
  // Contains all modules that correspond to a project and owns them
  map<string, unique_ptr<llvm::Module>> modules;
  // Maps function names to the module they are !defined! in
  map<string, string> functions;
  // Maps globals to the module they are !defined! in
  map<string, string> globals;
  // Maps an id to its corresponding instruction
  map<size_t, llvm::Instruction *> instructions;
  // Maps a function to its points-to graph
  map<string, unique_ptr<PointsToGraph>> ptgs;

  void buildFunctionModuleMapping(llvm::Module *M);
  void buildGlobalModuleMapping(llvm::Module *M);
  void buildIDModuleMapping(llvm::Module *M);
  void preprocessModule(llvm::Module *M);

public:
  // Constructs an empty ProjectIRDB
  ProjectIRDB();
  // Constructs a ProjectIRDB from a bunch of llvm IR files
  ProjectIRDB(const vector<string> &IRFiles);
  // Constructs a ProjectIRDB from a CompilationDatabase (only for simple
  // projects)
  ProjectIRDB(const clang::tooling::CompilationDatabase &CompileDB);
  // Constructs a ProjectIRDB from files wich may have to be compiled to llvm IR
  ProjectIRDB(const vector<string> &Files, vector<const char *> CompileArgs);
  ProjectIRDB(ProjectIRDB &&) = default;
  ~ProjectIRDB() = default;

  void preprocessIR();

  // add WPA support by providing a fat completely linked module
  void linkForWPA();
  // get a completely linked module for the WPA_MODE
  llvm::Module *getWPAModule();
  bool containsSourceFile(const string &src);
  bool empty();
  llvm::LLVMContext *getLLVMContext(const string &ModuleName);
  void insertModule(unique_ptr<llvm::Module> M);
  llvm::Module *getModule(const string &ModuleName);
  set<llvm::Module *> getAllModules() const;
  set<const llvm::Function *> getAllFunctions();
  set<string> getAllSourceFiles();
  size_t getNumberOfModules();
  llvm::Module *getModuleDefiningFunction(const string &FunctionName);
  llvm::Function *getFunction(const string &FunctionName);
  llvm::GlobalVariable *getGlobalVariable(const string &GlobalVariableName);
  llvm::Instruction *getInstruction(size_t id);
  size_t getInstructionID(const llvm::Instruction *I);
  PointsToGraph *getPointsToGraph(const string &FunctionName);
  void insertPointsToGraph(const string &FunctionName, PointsToGraph *ptg);
  void print();
  void exportPATBCJSON();
  string valueToPersistedString(const llvm::Value *V);
  const llvm::Value *persistedStringToValue(const string &StringRep);
};

#endif /* ANALYSIS_ProjectIRDB_HH_ */
