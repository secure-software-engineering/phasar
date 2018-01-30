#include "ProjectIRDB.h"

const set<string> ProjectIRDB::unknown_flags = {
    "-g", "-g3", "-pipe", "-fomit-frame-pointer", "-fstrict-aliasing",
    "-march=core2", "-fPIC", "-msse2", "-fvisibility=hidden",
    "-fno-strict-overflow", "-fstack-protector-all", "--param", "-fPIE",
    "-fno-default-inline"
    "-MF",
    "-fno-exceptions", "-fdiagnostics-color",
};

ProjectIRDB::ProjectIRDB() {}

ProjectIRDB::ProjectIRDB(const vector<string> &IRFiles) {
  setupHeaderSearchPaths();
  for (auto &File : IRFiles) {
    source_files.insert(File);
    // if we have a file that is already compiled to llvm ir
    if (File.find(".ll") != File.npos) {
      llvm::SMDiagnostic Diag;
      unique_ptr<llvm::LLVMContext> C(new llvm::LLVMContext);
      unique_ptr<llvm::Module> M = llvm::parseIRFile(File, Diag, *C);
      bool broken_debug_info = false;
      if (llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
        throw runtime_error(File + " could not be parsed correctly");
      }
      if (broken_debug_info) {
        cout << "caution: debug info is broken\n";
      }
      buildFunctionModuleMapping(M.get());
      buildGlobalModuleMapping(M.get());
      contexts.insert(make_pair(File, move(C)));
      modules.insert(make_pair(File, move(M)));
    } else {
      throw invalid_argument(File + " is not a valid llvm module");
    }
  }
}

ProjectIRDB::ProjectIRDB(const clang::tooling::CompilationDatabase &CompileDB) {
  setupHeaderSearchPaths();
  for (auto file : CompileDB.getAllFiles()) {
    auto compilecommands = CompileDB.getCompileCommands(file);
    for (auto compilecommand : compilecommands) {
      vector<const char *> args;
      // save the filename
      source_files.insert(compilecommand.Filename);
      // prepare the compile command for the clang compiler
      // the compile command is not sanitized yet
      args.push_back(compilecommand.Filename.c_str());
      for (size_t i = 2; i < compilecommand.CommandLine.size() - 1; ++i) {
        args.push_back(compilecommand.CommandLine[i].c_str());
      }
      compileAndAddToDB(args);
    }
  }
}

ProjectIRDB::ProjectIRDB(const vector<string> &Modules,
                         vector<const char *> CompileArgs) {
  setupHeaderSearchPaths();
  for (auto &Path : Modules) {
    source_files.insert(Path);
    // if we have a file that is already compiled to llvm ir
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
      // else we compile and then add to the database
      CompileArgs.insert(CompileArgs.begin(), Path.c_str());
      compileAndAddToDB(CompileArgs);
    }
  }
}

void ProjectIRDB::setupHeaderSearchPaths() {
  header_search_paths = splitString(
      readFile(ConfigurationDirectory + HeaderSearchPathsFileName), "\n");
  for (auto &path : header_search_paths) {
    path = string("-I") + path;
  }
}

void ProjectIRDB::compileAndAddToDB(vector<const char *> CompileCommand) {
  auto &lg = lg::get();
  // sanitize the compile command
  // kill all arguments that we are not interested in
  for (auto it = CompileCommand.begin(); it != CompileCommand.end();) {
    if (unknown_flags.count(string(*it))) {
      it = CompileCommand.erase(it);
    } else {
      ++it;
    }
  }
  // also remove the '-o some_filename.o' output options
  for (auto it = CompileCommand.begin(); it != CompileCommand.end();) {
    if (string(*it) == "-o") {
      it = CompileCommand.erase(it, it + 1);
    } else {
      ++it;
    }
  }
  // also remove the '-o some_filename.o' output options
  for (auto it = CompileCommand.begin(); it != CompileCommand.end();) {
    if (string(*it) == "-c") {
      it = CompileCommand.erase(it, it + 1);
    } else {
      ++it;
    }
  }
  // add the standard header search paths to the compile command
  for_each(header_search_paths.begin(), header_search_paths.end(),
           [&CompileCommand](string &path) {
             CompileCommand.push_back(path.c_str());
           });
  string commandstr;
  for (auto s : CompileCommand) {
    commandstr += s;
    commandstr += ' ';
  }
  BOOST_LOG_SEV(lg, INFO) << "compile with command: " << commandstr;
  unique_ptr<clang::CompilerInstance> ClangCompiler(
      new clang::CompilerInstance());
  clang::DiagnosticOptions *DiagOpts(new clang::DiagnosticOptions());
  clang::TextDiagnosticPrinter *DiagPrinterClient(
      new clang::TextDiagnosticPrinter(llvm::errs(), DiagOpts));
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(
      new clang::DiagnosticIDs());
  clang::DiagnosticsEngine *DiagEngine(
      new clang::DiagnosticsEngine(DiagID, DiagOpts, DiagPrinterClient, true));
  // prepare CodeGenAction and CompilerInvocation and compile!
  unique_ptr<clang::CodeGenAction> Action(new clang::EmitLLVMOnlyAction());
  // Awesome, this does not need to be a smart-pointer, because some idiot
  // thought it is a good
  // idea that the CompilerInstance the CompilerInvocation is handed to owns it
  // and CLEANS IT UP!
  clang::CompilerInvocation *CI(new clang::CompilerInvocation);
  clang::CompilerInvocation::CreateFromArgs(
      *CI, &CompileCommand[0], &CompileCommand[0] + CompileCommand.size(),
      *DiagEngine);
  ClangCompiler->setDiagnostics(DiagEngine);
  ClangCompiler->setInvocation(CI);
  if (!ClangCompiler->hasDiagnostics()) {
    cout << "compiler has no diagnostics engine\n";
  }
  if (!ClangCompiler->ExecuteAction(*Action)) {
    cout << "could not compile module!\n";
  }
  unique_ptr<llvm::Module> module = Action->takeModule();
  if (module != nullptr) {
    string name = module->getName().str();
    // check if module is alright
    bool broken_debug_info = false;
    if (llvm::verifyModule(*module, &llvm::errs(), &broken_debug_info)) {
      cout << "module is broken!\nabort!\n";
      DIE_HARD;
    }
    if (broken_debug_info) {
      cout << "debug info is broken\n";
    }
    buildFunctionModuleMapping(module.get());
    buildGlobalModuleMapping(module.get());
    contexts.insert(make_pair(
        name, unique_ptr<llvm::LLVMContext>(Action->takeLLVMContext())));
    modules.insert(make_pair(name, move(module)));
  } else {
    cout << "could not compile module!\nabort\n";
    DIE_HARD;
  }
}

void ProjectIRDB::preprocessModule(llvm::Module *M) {
  PAMM_FACTORY;
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Preprocess module: " << M->getModuleIdentifier();
  // TODO Have a look at this stuff from the future at some point in time
  /// PassManagerBuilder - This class is used to set up a standard
  /// optimization
  /// sequence for languages like C and C++, allowing some APIs to customize
  /// the
  /// pass sequence in various ways. A simple example of using it would be:
  ///
  ///  PassManagerBuilder Builder;
  ///  Builder.OptLevel = 2;
  ///  Builder.populateFunctionPassManager(FPM);
  ///  Builder.populateModulePassManager(MPM);
  ///
  /// In addition to setting up the basic passes, PassManagerBuilder allows
  /// frontends to vend a plugin API, where plugins are allowed to add
  /// extensions
  /// to the default pass manager.  They do this by specifying where in the
  /// pass
  /// pipeline they want to be added, along with a callback function that adds
  /// the pass(es).  For example, a plugin that wanted to add a loop
  /// optimization
  /// could do something like this:
  ///
  /// static void addMyLoopPass(const PMBuilder &Builder, PassManagerBase &PM)
  /// {
  ///   if (Builder.getOptLevel() > 2 && Builder.getOptSizeLevel() == 0)
  ///     PM.add(createMyAwesomePass());
  /// }
  ///   ...
  ///   Builder.addExtension(PassManagerBuilder::EP_LoopOptimizerEnd,
  ///                        addMyLoopPass);
  ///   ...
  // But for now, stick to what is well debugged
  START_TIMER("Pass " + M->getModuleIdentifier() + " Time");
  llvm::legacy::PassManager PM;
  if (VariablesMap["mem2reg"].as<bool>()) {
    llvm::FunctionPass *Mem2Reg = llvm::createPromoteMemoryToRegisterPass();
    PM.add(Mem2Reg);
  }
  GeneralStatisticsPass *GSP = new GeneralStatisticsPass();
  ValueAnnotationPass *VAP = new ValueAnnotationPass(M->getContext());
  // Mandatory passed for the alias analysis
  auto BasicAAWP = llvm::createBasicAAWrapperPass();
  auto TargetLibraryWP = new llvm::TargetLibraryInfoWrapperPass();
  // Optional, more precise alias analysis
  // auto ScopedNoAliasAAWP = llvm::createScopedNoAliasAAWrapperPass();
  // auto TBAAWP = llvm::createTypeBasedAAWrapperPass();
  // auto ObjCARCAAWP = llvm::createObjCARCAAWrapperPass();
  // auto SCEVAAWP = llvm::createSCEVAAWrapperPass();
  auto CFLAndersAAWP = llvm::createCFLAndersAAWrapperPass();
  // auto CFLSteensAAWP = llvm::createCFLSteensAAWrapperPass();
  // Add the passes
  PM.add(GSP);
  PM.add(VAP);
  PM.add(BasicAAWP);
  PM.add(TargetLibraryWP);
  // PM.add(ScopedNoAliasAAWP);
  // PM.add(TBAAWP);
  // PM.add(ObjCARCAAWP);
  // PM.add(SCEVAAWP);
  PM.add(CFLAndersAAWP);
  // PM.add(CFLSteensAAWP);
  PM.run(*M);
  STOP_TIMER("Pass " + M->getModuleIdentifier() + " Time");
  // just to be sure that none of the passes has messed up the module!
  bool broken_debug_info = false;
  if (llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
    BOOST_LOG_SEV(lg, CRITICAL) << "AnalysisController: module is broken!";
  }
  if (broken_debug_info) {
    BOOST_LOG_SEV(lg, WARNING) << "AnalysisController: debug info is broken.";
  }
  START_TIMER("PointsToAnalysisTime");
  // Obtain the very important alias analysis results
  // and construct the intra-procedural points-to graphs.
  for (auto &F : *M) {
    // When module-wise analysis is performed, declarations might occure
    // causing meaningless points-to graphs to be produced.
    if (!F.isDeclaration()) {
      llvm::BasicAAResult BAAResult =
          createLegacyPMBasicAAResult(*BasicAAWP, F);
      llvm::AAResults AARes =
          llvm::createLegacyPMAAResults(*BasicAAWP, F, BAAResult);
      insertPointsToGraph(F.getName().str(), new PointsToGraph(AARes, &F));
    }
  }
  STOP_TIMER("PointsToAnalysisTime");
  buildIDModuleMapping(M);
}

void ProjectIRDB::linkForWPA() {
  // Linking llvm modules:
  // Unfortunately linking between different contexts is currently not possible.
  // Therefore we must load all modules into one single context and then perform
  // the linkage. This is still very fast compared to compiling and
  // pre-processing
  // all modules.
  if (modules.size() > 1) {
    llvm::Module *MainMod = getModuleDefiningFunction("main");
    assert(MainMod && "could not find main function");
    for (auto &entry : modules) {
      // we do not want to link a module with itself!
      if (MainMod != entry.second.get()) {
        // reload the modules into the module containing the main function
        string IRBuffer;
        llvm::raw_string_ostream RSO(IRBuffer);
        llvm::WriteBitcodeToFile(entry.second.get(), RSO);
        RSO.flush();
        llvm::SMDiagnostic ErrorDiagnostics;
        unique_ptr<llvm::MemoryBuffer> MemBuffer =
            llvm::MemoryBuffer::getMemBuffer(IRBuffer);
        unique_ptr<llvm::Module> TmpMod =
            llvm::parseIR(*MemBuffer, ErrorDiagnostics, MainMod->getContext());
        bool broken_debug_info = false;
        if (llvm::verifyModule(*TmpMod, &llvm::errs(), &broken_debug_info)) {
          cout << "module is broken!\nabort!" << endl;
          DIE_HARD;
        }
        if (broken_debug_info) {
          cout << "debug info is broken" << endl;
        }
        // now we can safely perform the linking
        if (llvm::Linker::linkModules(*MainMod, move(TmpMod),
                                      llvm::Linker::LinkOnlyNeeded)) {
          cout << "ERROR when try to link modules for WPA module!" << endl;
          DIE_HARD;
        }
      }
    }
    // Update the IRDB reflecting that we now only need 'MainMod' and its
    // corresponding context!
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
    for (auto &entry : functions) {
      entry.second = MainMod->getModuleIdentifier();
    }
    // update globals
    for (auto &entry : globals) {
      entry.second = MainMod->getModuleIdentifier();
    }
    cout << "remaining contexts: " << contexts.size() << endl;
    cout << "remaining modules: " << modules.size() << endl;
    WPAMOD = MainMod;
  } else if (modules.size() == 1) {
    // In this case we only have one module anyway, so we do not have
    // to link at all. But we have to the the WPAMOD pointer!
    WPAMOD = modules.begin()->second.get();
  }
}

void ProjectIRDB::preprocessIR() {
  for (llvm::Module *M : getAllModules()) {
    preprocessModule(M);
  }
}

llvm::Module *ProjectIRDB::getWPAModule() {
  if (!WPAMOD)
    linkForWPA();
  return WPAMOD;
}

void ProjectIRDB::buildFunctionModuleMapping(llvm::Module *M) {
  for (auto &function : M->functions()) {
    if (!function.isDeclaration()) {
      functions[function.getName().str()] = M->getModuleIdentifier();
    }
  }
}

void ProjectIRDB::buildGlobalModuleMapping(llvm::Module *M) {
  for (auto &global : M->globals()) {
    globals[global.getName().str()] = M->getModuleIdentifier();
  }
}

void ProjectIRDB::buildIDModuleMapping(llvm::Module *M) {
  for (auto &F : *M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        instructions[stol(getMetaDataID(&I))] = &I;
      }
    }
  }
}

bool ProjectIRDB::containsSourceFile(const string &src) {
  return source_files.find(src) != source_files.end();
}

llvm::LLVMContext *ProjectIRDB::getLLVMContext(const string &name) {
  if (contexts.count(name))
    return contexts[name].get();
  return nullptr;
}

llvm::Module *ProjectIRDB::getModule(const string &name) {
  if (modules.count(name))
    return modules[name].get();
  return nullptr;
}

set<llvm::Module *> ProjectIRDB::getAllModules() const {
  set<llvm::Module *> ModuleSet;
  for (auto &entry : modules) {
    ModuleSet.insert(entry.second.get());
  }
  return ModuleSet;
}

size_t ProjectIRDB::getNumberOfModules() { return modules.size(); }

llvm::Module *ProjectIRDB::getModuleDefiningFunction(const string &name) {
  if (functions.count(name)) {
    return modules[functions[name]].get();
  }
  return nullptr;
}

llvm::Function *ProjectIRDB::getFunction(const string &name) {
  if (functions.count(name))
    return modules[functions[name]]->getFunction(name);
  return nullptr;
}

llvm::GlobalVariable *ProjectIRDB::getGlobalVariable(const string &name) {
  if (globals.count(name))
    return modules[globals[name]]->getGlobalVariable(name);
  return nullptr;
}

set<string> ProjectIRDB::getAllSourceFiles() { return source_files; }

llvm::Instruction *ProjectIRDB::getInstruction(size_t id) {
  if (instructions.count(id))
    return instructions[id];
  return nullptr;
}

size_t ProjectIRDB::getInstructionID(const llvm::Instruction *I) {
  size_t id = 0;
  if (auto MD = llvm::cast<llvm::MDString>(I->getMetadata(MetaDataKind)->getOperand(0))) {
    id = stol(MD->getString().str());
  }
  return id;
}

PointsToGraph *ProjectIRDB::getPointsToGraph(const string &name) {
  if (ptgs.count(name))
    return ptgs[name].get();
  return nullptr;
}

void ProjectIRDB::print() {
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

void ProjectIRDB::exportPATBCJSON() {
  cout << "ProjectIRDB::exportPATBCJSON\n";
}

string ProjectIRDB::valueToPersistedString(const llvm::Value *V) {
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
*/
  /**
          * @brief Creates a unique string representation for any given
   * llvm::Value.
          */
  if (isLLVMZeroValue(V)) {
    return ZeroValueInternalName;
  } else if (const llvm::Instruction *I =
                 llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str() + "." + getMetaDataID(I);
  } else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(V)) {
    return A->getParent()->getName().str() + ".f" + to_string(A->getArgNo());
  } else if (const llvm::GlobalValue *G =
                 llvm::dyn_cast<llvm::GlobalValue>(V)) {
    cout << "special case: WE ARE AN GLOBAL VARIABLE\n";
    cout << "all user:\n";
    for (auto User : V->users()) {
      if (const llvm::Instruction *I =
              llvm::dyn_cast<llvm::Instruction>(User)) {
        cout << I->getFunction()->getName().str() << "\n";
      }
    }
    return G->getName().str();
  } else if (llvm::isa<llvm::Value>(V)) {
    // In this case we should have an operand of an instruction which can be
    // identified by the instruction id and the operand index.
    cout << "special case: WE ARE AN OPERAND\n";
    // We should only have one user in this special case
    for (auto User : V->users()) {
      if (const llvm::Instruction *I =
              llvm::dyn_cast<llvm::Instruction>(User)) {
        for (unsigned idx = 0; idx < I->getNumOperands(); ++idx) {
          if (I->getOperand(idx) == V) {
            return I->getFunction()->getName().str() + "." + getMetaDataID(I) +
                   ".o." + to_string(idx);
          }
        }
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Value is of unexpected type.");
    return "";
  } else {
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Value is of unexpected type.");
    return "";
  }
}

const llvm::Value *ProjectIRDB::persistedStringToValue(const string &S) {
  /**
  * @brief Convertes the given string back into the llvm::Value it represents.
  * @return Pointer to the converted llvm::Value.
  */
  if (S == ZeroValueInternalName ||
      S.find(ZeroValueInternalName) != string::npos) {
    return new ZeroValue;
  } else if (S.find(".") == string::npos) {
    return getGlobalVariable(S);
  } else if (S.find(".f") != string::npos) {
    unsigned argno = stoi(S.substr(S.find(".f") + 2, S.size()));
    return getNthFunctionArgument(getFunction(S.substr(0, S.find(".f"))),
                                  argno);
  } else if (S.find(".o.") != string::npos) {
    unsigned i = S.find(".");
    unsigned j = S.find(".o.");
    unsigned instID = stoi(S.substr(i + 1, j));
    // cout << "FOUND instID: " << instID << "\n";
    unsigned opIdx = stoi(S.substr(j + 3, S.size()));
    // cout << "FOUND opIdx: " << to_string(opIdx) << "\n";
    llvm::Function *F = getFunction(S.substr(0, S.find(".")));
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (getMetaDataID(&I) == to_string(instID)) {
          return I.getOperand(opIdx);
        }
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("Operand not found.");
  } else if (S.find(".") != string::npos) {
    llvm::Function *F = getFunction(S.substr(0, S.find(".")));
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (getMetaDataID(&I) == S.substr(S.find(".") + 1, S.size())) {
          return &I;
        }
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("llvm::Instruction not found.");
  } else {
    UNRECOVERABLE_CXX_ERROR_UNCOND(
        "string cannot be translated into llvm::Value.");
  }
  return nullptr;
}

void ProjectIRDB::insertPointsToGraph(const string &FunctionName,
                                      PointsToGraph *ptg) {
  ptgs.insert(make_pair(FunctionName, unique_ptr<PointsToGraph>(ptg)));
}

set<const llvm::Function *> ProjectIRDB::getAllFunctions() {
  set<const llvm::Function *> funs;
  for (auto entry : functions) {
    const llvm::Function *f = modules[entry.second]->getFunction(entry.first);
    funs.insert(f);
  }
  return funs;
}

bool ProjectIRDB::empty() { return modules.empty(); }

void ProjectIRDB::insertModule(unique_ptr<llvm::Module> M) {
  source_files.insert(M->getModuleIdentifier());
  for (auto &F : *M) {
    functions.insert(make_pair(F.getName().str(), M->getModuleIdentifier()));
  }
  for (auto &G : M->globals()) {
    globals.insert(make_pair(G.getName().str(), M->getModuleIdentifier()));
  }
  buildFunctionModuleMapping(M.get());
  buildGlobalModuleMapping(M.get());
  buildIDModuleMapping(M.get());
  contexts.insert(make_pair(M->getModuleIdentifier(),
                            unique_ptr<llvm::LLVMContext>(&M->getContext())));
  modules.insert(make_pair(M->getModuleIdentifier(), move(M)));
}
