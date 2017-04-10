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
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include "../utils/utils.hh"
using namespace std;

class ProjectIRCompiledDB {
 public:
  // stores all source files that have been examined, in-memory only
  set<string> source_files;
  // contains all contexts for all modules and owns them, in-memory only
  map<string, unique_ptr<llvm::LLVMContext>> contexts;
  // contains all modules that correspond to a project and owns them, must be stored persistently
  map<string, unique_ptr<llvm::Module>> modules;
  // maps function names to the module they are defined in, must be stored persistently
  map<string, string> functions;
  // maps globals to the modulde they are defined in, must be stored persistently
  map<string, string> globals;
  // maps a id range of llvm value to the module they can be found in, must be stored persistently
  map<size_t, string> ids;
  // maps a module to its alias analysis results, in-memory only
  map<string, unique_ptr<llvm::AAResults>> aaresults;
  ProjectIRCompiledDB(const clang::tooling::CompilationDatabase& CompileDB);
  ProjectIRCompiledDB(const string Path, vector<const char*> CompileArgs);
  ~ProjectIRCompiledDB() = default;
  void buildFunctionModuleMapping();
  void buildGlobalModuleMapping();
  void buildIDModuleMapping();
  void print();
};

#endif /* ANALYSIS_PROJECTIRCOMPILEDDB_HH_ */
