#include "ProjectIRCompiledDB.hh"

ProjectIRCompiledDB::ProjectIRCompiledDB(
    const clang::tooling::CompilationDatabase& CompileDB) {
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(
      new clang::DiagnosticIDs());
  clang::DiagnosticsEngine Diags(DiagID, new clang::DiagnosticOptions());
  clang::CompilerInstance ClangCompiler;
  ClangCompiler.createDiagnostics();

  for (auto file : CompileDB.getAllFiles()) {
    auto compilecommands = CompileDB.getCompileCommands(file);
    vector<const char*> args;
    for (auto compilecommand : compilecommands) {
      // save the filename
      source_files.insert(compilecommand.Filename);
      // prepare the compile command for the clang compiler
      args.push_back(
          compilecommand.CommandLine[compilecommand.CommandLine.size() - 1]
              .c_str());
      // prepare CodeGenAction and Compiler invocation and compile!
      unique_ptr<clang::CodeGenAction> Action(new clang::EmitLLVMOnlyAction());
      unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
      clang::CompilerInvocation::CreateFromArgs(*CI, &args[0],
                                                &args[0] + args.size(), Diags);
      ClangCompiler.setInvocation(CI.get());
      if (!ClangCompiler.ExecuteAction(*Action)) {
        cout << "could not compile module!" << endl;
      }
      unique_ptr<llvm::Module> module = Action->takeModule();
      if (module != nullptr) {
        string name = module->getName().str();
        // check if module is alright
        bool broken_debug_info = false;
        if (llvm::verifyModule(*module, &llvm::errs(), &broken_debug_info)) {
          cout << "module is broken!" << endl;
        }
        if (broken_debug_info) {
          cout << "debug info is broken" << endl;
        }
        contexts.insert(make_pair(
            name, unique_ptr<llvm::LLVMContext>(Action->takeLLVMContext())));
        modules.insert(make_pair(name, move(module)));
      } else {
        cout << "could not compile module!" << endl;
      }
    }
  }
}

void ProjectIRCompiledDB::print() {
  for (auto& entry : modules) {
    cout << "front-end module: " << entry.first << endl;
    entry.second->dump();
  }
}