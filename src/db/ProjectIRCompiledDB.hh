#ifndef ANALYSIS_PROJECTIRCOMPILEDDB_HH_
#define ANALYSIS_PROJECTIRCOMPILEDDB_HH_

#include <clang/Basic/Diagnostic.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker/Linker.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include "../utils/utils.hh"
#include "../analysis/call-points-to_graph/PointsToGraph.hh"
using namespace std;

// forward declare the PointsToGraph
class PointsToGraph;

/**
 * 	@brief Owns important information regarding the LLVM IR code.
 *
 * 	This class owns the LLVM IR code of the project under analysis and some
 * 	very important information associated with the IR.
 * 	When an object of this class is destroyed it will clean up all IR related
 * 	stuff that is stored in it.
 */
class ProjectIRCompiledDB {
private:
	  llvm::Module* WPAMOD = nullptr;
	  void compileAndAddToDB(vector<const char *> CompileCommand);

 public:
  /// Stores all source files that have been examined, in-memory only.
  set<string> source_files;
  /// Contains all contexts for all modules and owns them, in-memory only.
  map<string, unique_ptr<llvm::LLVMContext>> contexts;
  /// Contains all modules that correspond to a project and owns them, must be stored persistently.
  map<string, unique_ptr<llvm::Module>> modules;
  /// Maps function names to the module they are defined in, must be stored persistently.
  map<string, string> functions;
  /// Maps globals to the module they are defined in, must be stored persistently.
  map<string, string> globals;
  /// Maps a id range of llvm value to the module they can be found in, must be stored persistently.
  map<size_t, string> ids;
  /// Maps a function to its points-to graph.
  map<string, unique_ptr<PointsToGraph>> ptgs;

  /**
   * 	@brief
   */
  ProjectIRCompiledDB(const clang::tooling::CompilationDatabase& CompileDB);

  /**
   * 	@brief
   */
  ProjectIRCompiledDB(const string Path, vector<const char*> CompileArgs);
  ~ProjectIRCompiledDB() = default;

  /**
   * 	@brief
   */
  void buildFunctionModuleMapping();

  /**
   * 	@brief
   */
  void buildGlobalModuleMapping();

  /**
   * 	@brief
   */
  void buildIDModuleMapping();
  // add WPA support by providing a fat completely linked module
  void linkForWPA();

  /**
   * 	@brief Returns a completely linked module for the WPA_MODE.
   * 	@return completely linked LLVM module
   */
  llvm::Module* getWPAModule();

   // add some useful functionality for querying the database
  bool containsSourceFile(const string& src);

  /**
   * 	@brief Returns a LLVM context associated to the given name.
   * 	@param name Name of the LLVM module.
   * 	@return Reference to the LLVM Context.
   */
  llvm::LLVMContext* getLLVMContext(const string& name);

  /**
   * 	@brief Returns a LLVM module.
   * 	@param name Name of the LLVM module.
   * 	@return Reference to the LLVM module.
   */
  llvm::Module* getModule(const string& name);

  /**
   * 	@brief Returns a LLVM module that contains the given function.
   * 	@param name Function identifier.
   * 	@return Reference to the LLVM module.
   */
  llvm::Module* getModuleContainingFunction(const string& name);

  /**
   * 	@brief Returns a LLVM function associated to the given name.
   * 	@param name Function identifier.
   * 	@return Reference to the LLVM function.
   */
  llvm::Function* getFunction(const string& name);

  /**
   * 	@brief Returns a LLVM global variable associated to the given name.
   * 	@param name Global variable identifier.
   * 	@return Reference to the LLVM global variable.
   */
  llvm::GlobalVariable* getGlobalVariable(const string& name);


  llvm::Instruction* getInstruction(size_t id);

  /**
   * 	@brief Returns the points-to graph of the given function.
   * 	@param Function identifier.
   * 	@return Reference to a PointsToGraph object.
   */
  PointsToGraph* getPointsToGraph(const string& name);

  /**
   * 	@brief Prints all modules and function identifier to the
   * 	       command-line.
   */
  void print();
};

#endif /* ANALYSIS_PROJECTIRCOMPILEDDB_HH_ */
