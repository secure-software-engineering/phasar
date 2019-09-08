/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <cassert>
#include <iostream>

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/CFLAndersAliasAnalysis.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <boost/filesystem.hpp>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/Passes/GeneralStatisticsPass.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/Utils/EnumFlags.h>
#include <phasar/Utils/IO.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMMMacros.h>

using namespace psr;
using namespace std;

namespace psr {

const std::set<std::string> ProjectIRDB::unknown_flags = {
    "-g",
    "-g3",
    "-pipe",
    "-fomit-frame-pointer",
    "-fstrict-aliasing",
    "-march=core2",
    "-fPIC",
    "-msse2",
    "-fvisibility=hidden",
    "-fno-strict-overflow",
    "-fstack-protector-all",
    "--param",
    "-fPIE",
    "-fno-default-inline"
    "-MF",
    "-fno-exceptions",
    "-fdiagnostics-color",
};

ProjectIRDB::ProjectIRDB(enum IRDBOptions Opt) : Options(Opt) {}

ProjectIRDB::ProjectIRDB(const std::vector<std::string> &IRFiles,
                         enum IRDBOptions Opt)
    : Options(Opt) {
  for (const auto &File : IRFiles) {
    source_files.insert(File);
    // if we have a file that is already compiled to llvm ir
    if ((File.find(".ll") != File.npos || File.find(".bc") != File.npos) &&
        boost::filesystem::exists(File)) {
      llvm::SMDiagnostic Diag;
      std::unique_ptr<llvm::LLVMContext> C(new llvm::LLVMContext);
      std::unique_ptr<llvm::Module> M = llvm::parseIRFile(File, Diag, *C);
      bool broken_debug_info = false;
      if (M.get() == nullptr)
        Diag.print(File.c_str(), llvm::errs());
      /* Crash in presence of llvm-3.9.1 module (segfault) */
      if (M.get() == nullptr ||
          llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
        throw std::runtime_error(File + " could not be parsed correctly");
      }
      if (broken_debug_info) {
        std::cout << "caution: debug info is broken\n";
      }

      buildFunctionModuleMapping(M.get());
      buildGlobalModuleMapping(M.get());
      contexts.insert(std::make_pair(File, std::move(C)));
      modules.insert(std::make_pair(File, std::move(M)));
    } else {
      throw std::invalid_argument(File + " is not a valid llvm module");
    }
  }
  cout << "All modules loaded\n";
}

ProjectIRDB::~ProjectIRDB() {
  // if the IRDB doesn't own the given pointers, they have to be released before
  // destruction
  if (Options & IRDBOptions::OWNSNOT) {
    // release pointers

    for (auto &elem : contexts) {
      elem.second.release();
    }

    for (auto &elem : modules) {
      elem.second.release();
    }
  }
}

void ProjectIRDB::setupHeaderSearchPaths() {
  header_search_paths =
      splitString(readFile(PhasarConfig::ConfigurationDirectory() +
                           PhasarConfig::HeaderSearchPathsFileName()),
                  "\n");
  for (auto &path : header_search_paths) {
    path = std::string("-I") + path;
  }
}

void ProjectIRDB::preprocessModule(llvm::Module *M) {
  // WARNING: Activating passes lead to higher time in llvmIRToString
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  // add moduleID to timer name if performing MWA!
  START_TIMER("LLVM Passes", PAMM_SEVERITY_LEVEL::Full);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Preprocess module: " << M->getModuleIdentifier());

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
  llvm::legacy::PassManager PM;
  if (Options & IRDBOptions::MEM2REG) {
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
  // just to be sure that none of the passes has messed up the module!
  bool broken_debug_info = false;
  if (M == nullptr ||
      llvm::verifyModule(*M, &llvm::errs(), &broken_debug_info)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, CRITICAL)
                  << "AnalysisController: module is broken!");
  }
  if (broken_debug_info) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, WARNING)
                  << "AnalysisController: debug info is broken.");
  }
  for (auto RR : GSP->getRetResInstructions()) {
    ret_res_instructions.insert(RR);
  }
  for (auto A : GSP->getAllocaInstructions()) {
    alloca_instructions.insert(A);
  }
  // Obtain the allocated types found in the module
  allocated_types = GSP->getAllocatedTypes();
  STOP_TIMER("LLVM Passes", PAMM_SEVERITY_LEVEL::Full);
  cout << "PTG construction ...\n";
  START_TIMER("PTG Construction", PAMM_SEVERITY_LEVEL::Core);
  // Obtain the very important alias analysis results
  // and construct the intra-procedural points-to graphs.
  REG_COUNTER("GS Pointer", 0, PAMM_SEVERITY_LEVEL::Core)
  for (auto &F : *M) {
    // When module-wise analysis is performed, declarations might occure
    // causing meaningless points-to graphs to be produced.
    if (!F.isDeclaration()) {
      llvm::BasicAAResult BAAResult =
          createLegacyPMBasicAAResult(*BasicAAWP, F);
      llvm::AAResults AARes =
          llvm::createLegacyPMAAResults(*BasicAAWP, F, BAAResult);
      // This line is a major slowdown
      // The problem comes from the generation of PtG which is far too slow
      // due to the use of llvmIRToString (without it, the generation of PtG is
      // very acceptable)
      insertPointsToGraph(F.getName().str(), new PointsToGraph(AARes, &F));
    }
  }
  STOP_TIMER("PTG Construction", PAMM_SEVERITY_LEVEL::Core);
  cout << "PTG construction ended\n";

  buildIDModuleMapping(M);
}

void ProjectIRDB::linkForWPA() {
  // Linking llvm modules:
  // Unfortunately linking between different contexts is currently not possible.
  // Therefore we must load all modules into one single context and then perform
  // the linkage. This is still very fast compared to compiling and
  // pre-processing
  // all modules.
  // auto &lg = lg::get();
  if (modules.size() > 1) {
    llvm::Module *MainMod = getModuleDefiningFunction("main");
    assert(MainMod && "could not find main function");
    for (auto &entry : modules) {
      // we do not want to link a module with itself!
      if (MainMod != entry.second.get()) {
        // reload the modules into the module containing the main function
        std::string IRBuffer;
        llvm::raw_string_ostream RSO(IRBuffer);
        llvm::WriteBitcodeToFile(*entry.second.get(), RSO);
        RSO.flush();
        llvm::SMDiagnostic ErrorDiagnostics;
        std::unique_ptr<llvm::MemoryBuffer> MemBuffer =
            llvm::MemoryBuffer::getMemBuffer(IRBuffer);
        std::unique_ptr<llvm::Module> TmpMod =
            llvm::parseIR(*MemBuffer, ErrorDiagnostics, MainMod->getContext());
        bool broken_debug_info = false;
        if (TmpMod.get() == nullptr ||
            llvm::verifyModule(*TmpMod, &llvm::errs(), &broken_debug_info)) {
          std::cout << "module is broken!\nabort!" << std::endl;
          DIE_HARD;
        }
        if (broken_debug_info) {
          std::cout << "debug info is broken" << std::endl;
        }
        // now we can safely perform the linking
        if (llvm::Linker::linkModules(*MainMod, std::move(TmpMod),
                                      llvm::Linker::LinkOnlyNeeded)) {
          std::cout << "ERROR when trying to link modules for WPA module!"
                    << std::endl;
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
    for (auto &entry : functionToModuleMap) {
      entry.second = MainMod->getModuleIdentifier();
    }
    // update globals
    for (auto &entry : globals) {
      entry.second = MainMod->getModuleIdentifier();
    }
    std::cout << "remaining contexts: " << contexts.size() << std::endl;
    std::cout << "remaining modules: " << modules.size() << std::endl;
    WPAMOD = MainMod;
  } else if (modules.size() == 1) {
    // In this case we only have one module anyway, so we do not have
    // to link at all. But we have to update the WPAMOD pointer!
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
      functionToModuleMap[function.getName().str()] = M->getModuleIdentifier();
      functions.insert(&function);
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

bool ProjectIRDB::containsSourceFile(const std::string &src) {
  return source_files.find(src) != source_files.end();
}

llvm::LLVMContext *ProjectIRDB::getLLVMContext(const std::string &name) {
  if (contexts.count(name))
    return contexts[name].get();
  return nullptr;
}

llvm::Module *ProjectIRDB::getModule(const std::string &name) {
  if (modules.count(name))
    return modules[name].get();
  return nullptr;
}

std::size_t ProjectIRDB::getNumberOfModules() { return modules.size(); }

llvm::Module *ProjectIRDB::getModuleDefiningFunction(const std::string &name) {
  if (functionToModuleMap.count(name)) {
    return modules[functionToModuleMap[name]].get();
  }
  return nullptr;
}

llvm::Function *ProjectIRDB::getFunction(const std::string &name) {
  if (functionToModuleMap.count(name))
    return modules[functionToModuleMap[name]]->getFunction(name);
  return nullptr;
}

llvm::GlobalVariable *ProjectIRDB::getGlobalVariable(const std::string &name) {
  if (globals.count(name))
    return modules[globals[name]]->getGlobalVariable(name);
  return nullptr;
}

std::set<std::string> ProjectIRDB::getAllSourceFiles() { return source_files; }

llvm::Instruction *ProjectIRDB::getInstruction(std::size_t id) {
  if (instructions.count(id))
    return instructions[id];
  return nullptr;
}

std::size_t ProjectIRDB::getInstructionID(const llvm::Instruction *I) {
  std::size_t id = 0;
  if (auto MD = llvm::cast<llvm::MDString>(
          I->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))) {
    id = stol(MD->getString().str());
  }
  return id;
}

PointsToGraph *ProjectIRDB::getPointsToGraph(const std::string &name) {
  if (ptgs.count(name))
    return ptgs[name].get();
  return nullptr;
}

PointsToGraph *ProjectIRDB::getPointsToGraph(const std::string &name) const {
  if (ptgs.count(name))
    return ptgs.at(name).get();
  return nullptr;
}

void ProjectIRDB::print() {
  std::cout << "modules:" << std::endl;
  for (auto &entry : modules) {
    std::cout << "front-end module: " << entry.first << std::endl;
    llvm::outs() << *entry.second;
  }
  std::cout << "functions:" << std::endl;
  for (auto entry : functionToModuleMap) {
    std::cout << entry.first << " defined in module " << entry.second
              << std::endl;
  }
}

void ProjectIRDB::emitPreprocessedIR(std::ostream &os, bool shortenIR) {
  for (auto &entry : modules) {
    os << "IR module: " << entry.first << '\n';
    // print globals
    for (auto &glob : entry.second->globals()) {
      if (shortenIR) {
        os << llvmIRToShortString(&glob);
      } else {
        os << llvmIRToString(&glob);
      }
      os << '\n';
    }
    os << '\n';
    for (auto F : getAllFunctions()) {
      if (getModuleDefiningFunction(F->getName().str())
              ->getModuleIdentifier() == entry.first) {
        os << F->getName().str() << " {\n";
        for (auto &BB : *F) {
          // do not print the label of the first BB
          if (BB.getPrevNode()) {
            std::string BBLabel;
            llvm::raw_string_ostream RSO(BBLabel);
            BB.printAsOperand(RSO, false);
            RSO.flush();
            os << "\n<label " << BBLabel << ">\n";
          }
          // print all instructions
          for (auto &I : BB) {
            os << "  ";
            if (shortenIR) {
              os << llvmIRToShortString(&I);
            } else {
              os << llvmIRToString(&I);
            }
            os << '\n';
          }
        }
        os << "}\n\n";
      }
    }
    os << '\n';
  }
}

void ProjectIRDB::exportPATBCJSON() {
  std::cout << "ProjectIRDB::exportPATBCJSON\n";
}

std::string ProjectIRDB::valueToPersistedString(const llvm::Value *V) {
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(V)) {
    return LLVMZeroValue::getInstance()->getName();
  } else if (const llvm::Instruction *I =
                 llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str() + "." + getMetaDataID(I);
  } else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(V)) {
    return A->getParent()->getName().str() + ".f" + to_string(A->getArgNo());
  } else if (const llvm::GlobalValue *G =
                 llvm::dyn_cast<llvm::GlobalValue>(V)) {
    std::cout << "special case: WE ARE AN GLOBAL VARIABLE\n";
    std::cout << "all user:\n";
    for (auto User : V->users()) {
      if (const llvm::Instruction *I =
              llvm::dyn_cast<llvm::Instruction>(User)) {
        std::cout << I->getFunction()->getName().str() << "\n";
      }
    }
    return G->getName().str();
  } else if (llvm::isa<llvm::Value>(V)) {
    // In this case we should have an operand of an instruction which can be
    // identified by the instruction id and the operand index.
    std::cout << "special case: WE ARE AN OPERAND\n";
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

const llvm::Value *ProjectIRDB::persistedStringToValue(const std::string &S) {
  if (S.find(LLVMZeroValue::getInstance()->getName()) != std::string::npos) {
    return LLVMZeroValue::getInstance();
  } else if (S.find(".") == std::string::npos) {
    return getGlobalVariable(S);
  } else if (S.find(".f") != std::string::npos) {
    unsigned argno = stoi(S.substr(S.find(".f") + 2, S.size()));
    return getNthFunctionArgument(getFunction(S.substr(0, S.find(".f"))),
                                  argno);
  } else if (S.find(".o.") != std::string::npos) {
    unsigned i = S.find(".");
    unsigned j = S.find(".o.");
    unsigned instID = stoi(S.substr(i + 1, j));
    // std::cout << "FOUND instID: " << instID << "\n";
    unsigned opIdx = stoi(S.substr(j + 3, S.size()));
    // std::cout << "FOUND opIdx: " << to_string(opIdx) << "\n";
    llvm::Function *F = getFunction(S.substr(0, S.find(".")));
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (getMetaDataID(&I) == to_string(instID)) {
          return I.getOperand(opIdx);
        }
      }
    }
    UNRECOVERABLE_CXX_ERROR_UNCOND("Operand not found.");
  } else if (S.find(".") != std::string::npos) {
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

void ProjectIRDB::insertPointsToGraph(const std::string &FunctionName,
                                      PointsToGraph *ptg) {
  ptgs.insert(
      std::make_pair(FunctionName, std::unique_ptr<PointsToGraph>(ptg)));
}

std::set<const llvm::Value *> ProjectIRDB::getAllocaInstructions() {
  return alloca_instructions;
}

std::set<const llvm::Instruction *> ProjectIRDB::getRetResInstructions() {
  return ret_res_instructions;
}

std::set<const llvm::Function *> ProjectIRDB::getAllFunctions() {
  if (functions.size() == 0) {
    auto &lg = lg::get();
    for (const auto &entry : functionToModuleMap) {
      const llvm::Function *f = modules[entry.second]->getFunction(entry.first);
      if (f == nullptr) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, WARNING)
                      << entry.first << " is not contained in the module\n");
      } else
        functions.insert(f);
    }
  }
  return functions;
}

bool ProjectIRDB::empty() { return modules.empty(); }

void ProjectIRDB::insertModule(std::unique_ptr<llvm::Module> M) {
  source_files.insert(M->getModuleIdentifier());
  for (auto &F : *M) {
    functionToModuleMap.insert(
        std::make_pair(F.getName().str(), M->getModuleIdentifier()));
  }
  for (auto &G : M->globals()) {
    globals.insert(std::make_pair(G.getName().str(), M->getModuleIdentifier()));
  }
  buildFunctionModuleMapping(M.get());
  buildGlobalModuleMapping(M.get());
  buildIDModuleMapping(M.get());
  contexts.insert(
      std::make_pair(M->getModuleIdentifier(),
                     std::unique_ptr<llvm::LLVMContext>(&M->getContext())));
  modules.insert(std::make_pair(M->getModuleIdentifier(), std::move(M)));
}

set<const llvm::Type *> ProjectIRDB::getAllocatedTypes() {
  return allocated_types;
}

string
ProjectIRDB::getGlobalVariableModuleName(const string &GlobalVariableName) {
  if (globals.count(GlobalVariableName)) {
    return globals[GlobalVariableName];
  }
  return "";
}

set<const llvm::Value *> ProjectIRDB::getAllMemoryLocations() {
  // get all stack and heap alloca instructions
  set<const llvm::Value *> allMemoryLoc = getAllocaInstructions();
  set<string> IgnoredGlobalNames = {"llvm.used",
                                    "llvm.compiler.used",
                                    "llvm.global_ctors",
                                    "llvm.global_dtors",
                                    "vtable",
                                    "typeinfo"};
  // add global varibales to the memory location set, except the llvm
  // intrinsic global variables
  for (auto M : getAllModules()) {
    for (auto &GV : M->globals()) {
      if (GV.hasName()) {
        string GVName = cxx_demangle(GV.getName().str());
        if (!IgnoredGlobalNames.count(GVName.substr(0, GVName.find(' ')))) {
          allMemoryLoc.insert(&GV);
        }
      }
    }
  }
  return allMemoryLoc;
}
} // namespace psr
