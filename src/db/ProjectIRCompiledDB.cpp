#include "ProjectIRCompiledDB.hh"

ProjectIRCompiledDB::ProjectIRCompiledDB(
    const clang::tooling::CompilationDatabase &CompileDB) {
  for (auto file : CompileDB.getAllFiles()) {
    auto compilecommands = CompileDB.getCompileCommands(file);
    for (auto compilecommand : compilecommands) {
      vector<const char *> args;
      // save the filename
      source_files.insert(compilecommand.Filename);
      // prepare the compile command for the clang compiler
      args.push_back(
          compilecommand.CommandLine[compilecommand.CommandLine.size() - 1]
              .c_str());
      // create a diagnosticengine and the compiler instance that we use for
      // compilation
      clang::DiagnosticOptions DiagOpts;
      clang::TextDiagnosticPrinter *DiagPrinterClient =
          new clang::TextDiagnosticPrinter(llvm::errs(), &DiagOpts);
      llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(
          new clang::DiagnosticIDs());
      clang::DiagnosticsEngine Diags(DiagID, &DiagOpts, DiagPrinterClient,
                                     false);
      clang::CompilerInstance *ClangCompiler = new clang::CompilerInstance();
      ClangCompiler->createDiagnostics();
      // prepare CodeGenAction and Compiler invocation and compile!
      unique_ptr<clang::CodeGenAction> Action(new clang::EmitLLVMOnlyAction());
      unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
      clang::CompilerInvocation::CreateFromArgs(*CI, &args[0],
                                                &args[0] + args.size(), Diags);
      ClangCompiler->setDiagnostics(&Diags);
      ClangCompiler->setInvocation(CI.get());
      if (!ClangCompiler->hasDiagnostics()) {
        cout << "compiler has no diagnostics engine" << endl;
      }
      if (!ClangCompiler->ExecuteAction(*Action)) {
        cout << "could not compile module!" << endl;
      }
      unique_ptr<llvm::Module> module = Action->takeModule();
      if (module != nullptr) {
        string name = module->getName().str();
        // check if module is alright
        bool broken_debug_info = false;
        if (llvm::verifyModule(*module, &llvm::errs(), &broken_debug_info)) {
          cout << "module is broken!\nabort!" << endl;
          DIE_HARD;
        }
        if (broken_debug_info) {
          cout << "debug info is broken" << endl;
        }
        contexts.insert(make_pair(
            name, unique_ptr<llvm::LLVMContext>(Action->takeLLVMContext())));
        modules.insert(make_pair(name, move(module)));
      } else {
        cout << "could not compile module!\nabort" << endl;
        DIE_HARD;
      }
    }
  }
  buildFunctionModuleMapping();
  buildGlobalModuleMapping();
}

ProjectIRCompiledDB::ProjectIRCompiledDB(const string Path,
                                         vector<const char *> CompileArgs) {
  source_files.insert(Path);
  if (Path.find(".ll") != Path.npos) {
    llvm::SMDiagnostic Diag;
    unique_ptr<llvm::LLVMContext> C(new llvm::LLVMContext);
    unique_ptr<llvm::Module> M = llvm::parseIRFile(Path, Diag, *C);
    bool broken_debug_info = false;
    if (llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
      cout << "error: module not valid\n";
      DIE_HARD;
    }
    if (broken_debug_info) {
      cout << "caution: debug info is broken\n";
    }
    contexts.insert(make_pair(Path, move(C)));
    modules.insert(make_pair(Path, move(M)));
  } else {
    CompileArgs.insert(CompileArgs.begin(), Path.c_str());
    // create a diagnosticengine and the compiler instance that we use for
    // compilation
    clang::DiagnosticOptions DiagOpts;
    clang::TextDiagnosticPrinter *DiagPrinterClient =
        new clang::TextDiagnosticPrinter(llvm::errs(), &DiagOpts);
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(
        new clang::DiagnosticIDs());
    clang::DiagnosticsEngine Diags(DiagID, &DiagOpts, DiagPrinterClient, false);
    clang::CompilerInstance *ClangCompiler = new clang::CompilerInstance();
    ClangCompiler->createDiagnostics();
    // prepare CodeGenAction and Compiler invocation and compile!
    unique_ptr<clang::CodeGenAction> Action(new clang::EmitLLVMOnlyAction());
    unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
    clang::CompilerInvocation::CreateFromArgs(*CI, &CompileArgs[0],
                                              &CompileArgs[0] + CompileArgs.size(), Diags);
    ClangCompiler->setDiagnostics(&Diags);
    ClangCompiler->setInvocation(CI.get());
    if (!ClangCompiler->hasDiagnostics()) {
      cout << "compiler has no diagnostics engine" << endl;
    }
    if (!ClangCompiler->ExecuteAction(*Action)) {
      cout << "could not compile module!" << endl;
    }
    unique_ptr<llvm::Module> module = Action->takeModule();
    if (module != nullptr) {
      string name = module->getName().str();
      // check if module is alright
      bool broken_debug_info = false;
      if (llvm::verifyModule(*module, &llvm::errs(), &broken_debug_info)) {
        cout << "module is broken!\nabort!" << endl;
        DIE_HARD;
      }
      if (broken_debug_info) {
        cout << "debug info is broken" << endl;
      }
      contexts.insert(make_pair(
          name, unique_ptr<llvm::LLVMContext>(Action->takeLLVMContext())));
      modules.insert(make_pair(name, move(module)));
    } else {
      cout << "could not compile module!\nabort" << endl;
      DIE_HARD;
    }
  }
  buildFunctionModuleMapping();
  buildGlobalModuleMapping();
}

void ProjectIRCompiledDB::linkForWPA() {
	// Linking llvm modules:
	// Unfortunately linking between different contexts is currently not possible.
	// Therefore we must load all modules into one single context and then perform
	// the linkage. This is still very fast compared to compiling and pre-processing
	// all modules.
	llvm::Module* MainMod = getModuleContainingFunction("main");
	if (!MainMod) {
		cout << "could not find main() function!\n";
		HEREANDNOW;
		DIE_HARD;
	}
	for (auto& entry : modules) {
		// we do not want to link a module with itself!
		if (MainMod != entry.second.get()) {
			// reload the modules into the module containing the main function
			string IRBuffer;
			llvm::raw_string_ostream RSO(IRBuffer);
			llvm::WriteBitcodeToFile(entry.second.get(), RSO);
			RSO.flush();
			llvm::SMDiagnostic ErrorDiagnostics;
			unique_ptr<llvm::MemoryBuffer> MemBuffer = llvm::MemoryBuffer::getMemBuffer(IRBuffer);
			unique_ptr<llvm::Module> TmpMod = llvm::parseIR(*MemBuffer, ErrorDiagnostics, MainMod->getContext());
			bool broken_debug_info = false;
			if (llvm::verifyModule(*TmpMod, &llvm::errs(), &broken_debug_info)) {
				cout << "module is broken!\nabort!" << endl;
				DIE_HARD;
			}
			if (broken_debug_info) {
				cout << "debug info is broken" << endl;
			}
			// now we can safely perform the linking
			if (llvm::Linker::linkModules(*MainMod, move(TmpMod), llvm::Linker::LinkOnlyNeeded)) {
				cout << "ERROR when try to link modules for WPA module!" << endl;
				DIE_HARD;
			}
		}
	}
	// Update the IRDB reflecting that we now only need 'MainMod' and its corresponding context!
  // delete every other module
  for (auto it = modules.begin(); it != modules.end();) {
  	if (it->second.get() != MainMod) {
  		it = modules.erase(it);
  	} else {
  		++it;
  	}
  }
	// delete every other context
  for (auto it = contexts.begin(); it != contexts.end();) {
  	if (it->second.get() != &MainMod->getContext()) {
  		it = contexts.erase(it);
  	} else {
  		++it;
  	}
  }
  // update functions
  for (auto& entry : functions) {
  	entry.second = MainMod->getModuleIdentifier();
  }
  // update globals
  for (auto& entry : globals) {
  	entry.second = MainMod->getModuleIdentifier();
  }
  cout << "remaining contexts: " << contexts.size() << endl;
  cout << "remaining modules: " << modules.size() << endl;
	WPAMOD = MainMod;
}

llvm::Module* ProjectIRCompiledDB::getWPAModule() {
	if (!WPAMOD)
		linkForWPA();
	return WPAMOD;
}

void ProjectIRCompiledDB::buildFunctionModuleMapping() {
  for (auto &entry : modules) {
    const llvm::Module *M = entry.second.get();
    for (auto &function : M->functions()) {
      if (!function.isDeclaration()) {
        functions[function.getName().str()] = M->getModuleIdentifier();
      }
    }
  }
}

void ProjectIRCompiledDB::buildGlobalModuleMapping() {
  for (auto& entry : modules) {
    const llvm::Module *M = entry.second.get();
    for (auto& global : M->globals()) {
        globals[global.getName().str()] = M->getModuleIdentifier();
    }
  }
}

void ProjectIRCompiledDB::buildIDModuleMapping() {
	// determine first instruction of module
	// determine last instruction of module (user reverse module iterator)
}

bool ProjectIRCompiledDB::containsSourceFile(const string& src) {
	return source_files.find(src) != source_files.end();
}

llvm::LLVMContext* ProjectIRCompiledDB::getLLVMContext(const string& name) {
	return contexts[name].get();
}

llvm::Module* ProjectIRCompiledDB::getModule(const string& name) {
	return modules[name].get();
}

llvm::Module* ProjectIRCompiledDB::getModuleContainingFunction(const string& name) {
	return modules[functions[name]].get();
}

llvm::Function* ProjectIRCompiledDB::getFunction(const string& name) {
	return modules[functions[name]]->getFunction(name);
}

llvm::GlobalVariable* ProjectIRCompiledDB::getGlobalVariable(const string& name) {
	return modules[globals[name]]->getGlobalVariable(name);
}

PointsToGraph* ProjectIRCompiledDB::getPointsToGraph(const string& name) {
	return ptgs[name].get();
}

void ProjectIRCompiledDB::print() {
  cout << "modules:" << endl;
  for (auto &entry : modules) {
    cout << "front-end module: " << entry.first << endl;
    entry.second->dump();
  }
  cout << "functions:" << endl;
  for (auto entry : functions) {
    cout << entry.first << " defined in module " << entry.second << endl;
  }
}
