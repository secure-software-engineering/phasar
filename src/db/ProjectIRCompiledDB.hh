#ifndef ANALYSIS_PROJECTIRCOMPILEDDB_HH_
#define ANALYSIS_PROJECTIRCOMPILEDDB_HH_

#include <clang/Basic/Diagnostic.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
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
  set<string> source_files;
  // contains all contexts for all modules and owns them
  map<string, unique_ptr<llvm::LLVMContext>> contexts;
  // contains all modules that correspond to a project and owns them
  map<string, unique_ptr<llvm::Module>> modules;
  // maps function names to the module they are defined in
  map<string, const llvm::Module*> functions;
  ProjectIRCompiledDB(const clang::tooling::CompilationDatabase& CompileDB);
  ~ProjectIRCompiledDB() = default;
  void createFunctionModuleMapping();
  void print();
};

#endif /* ANALYSIS_PROJECTIRCOMPILEDDB_HH_ */