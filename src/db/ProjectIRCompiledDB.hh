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
using namespace std;

class ProjectIRCompiledDB {
 public:
  set<string> source_files;
  map<string, unique_ptr<llvm::LLVMContext>> contexts;
  map<string, unique_ptr<llvm::Module>> modules;
  ProjectIRCompiledDB(const clang::tooling::CompilationDatabase& CompileDB);
  ~ProjectIRCompiledDB() = default;
  void print();
};

#endif /* ANALYSIS_PROJECTIRCOMPILEDDB_HH_ */