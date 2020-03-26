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
#include <string>

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/Utils.h"

#include "boost/filesystem.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

using namespace psr;
using namespace std;

namespace psr {

ProjectIRDB::ProjectIRDB(IRDBOptions Options) : Options(Options) {}

ProjectIRDB::ProjectIRDB(const std::vector<std::string> &IRFiles,
                         IRDBOptions Options)
    : ProjectIRDB(Options | IRDBOptions::OWNS) {
  for (const auto &File : IRFiles) {
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
      Modules.insert(std::make_pair(File, std::move(M)));
      Contexts.push_back(std::move(C));
    } else {
      throw std::invalid_argument(File + " is not a valid llvm module");
    }
  }
  if (Options & IRDBOptions::WPA) {
    linkForWPA();
  }
  preprocessAllModules();
}

ProjectIRDB::ProjectIRDB(const std::vector<llvm::Module *> &Modules,
                         IRDBOptions Options)
    : ProjectIRDB(Options) {
  for (auto M : Modules) {
    insertModule(M);
  }
  if (Options & IRDBOptions::WPA) {
    linkForWPA();
  }
}

ProjectIRDB::~ProjectIRDB() {
  // release resources if IRDB does not own
  if (!(Options & IRDBOptions::OWNS)) {
    for (auto &Context : Contexts) {
      Context.release();
    }
    for (auto &[File, Module] : Modules) {
      Module.release();
    }
  }
}

void ProjectIRDB::preprocessModule(llvm::Module *M) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  // add moduleID to timer name if performing MWA!
  START_TIMER("LLVM Passes", PAMM_SEVERITY_LEVEL::Full);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Preprocess module: " << M->getModuleIdentifier());
  llvm::PassBuilder PB;
  llvm::ModuleAnalysisManager MAM;
  // register the GeneralStaticsPass analysis pass to the ModuleAnalysisManager
  // such that we can query its results later on
  GeneralStatisticsAnalysis GSP;
  MAM.registerPass([&]() { return std::move(GSP); });
  PB.registerModuleAnalyses(MAM);
  llvm::ModulePassManager MPM;
  // add the transformation pass ValueAnnotationPass
  MPM.addPass(ValueAnnotationPass());
  // just to be sure that none of the passes messed up the module!
  MPM.addPass(llvm::VerifierPass());
  MPM.run(*M, MAM);
  // retrieve data from the GeneralStatisticsAnalysis registered earlier
  auto GSPResult = MAM.getResult<GeneralStatisticsAnalysis>(*M);
  auto Allocas = GSPResult.getAllocaInstructions();
  AllocaInstructions.insert(Allocas.begin(), Allocas.end());
  auto ATypes = GSPResult.getAllocatedTypes();
  AllocatedTypes.insert(ATypes.begin(), ATypes.end());
  auto RRInsts = GSPResult.getRetResInstructions();
  RetOrResInstructions.insert(RRInsts.begin(), RRInsts.end());
  STOP_TIMER("LLVM Passes", PAMM_SEVERITY_LEVEL::Full);
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
  if (Modules.size() > 1) {
    llvm::Module *MainMod = getModuleDefiningFunction("main");
    assert(MainMod && "could not find main function");
    for (auto &[File, Module] : Modules) {
      // we do not want to link a module with itself!
      if (MainMod != Module.get()) {
        // reload the modules into the module containing the main function
        std::string IRBuffer;
        llvm::raw_string_ostream RSO(IRBuffer);
        llvm::WriteBitcodeToFile(*Module.get(), RSO);
        RSO.flush();
        llvm::SMDiagnostic ErrorDiagnostics;
        std::unique_ptr<llvm::MemoryBuffer> MemBuffer =
            llvm::MemoryBuffer::getMemBuffer(IRBuffer);
        std::unique_ptr<llvm::Module> TmpMod =
            llvm::parseIR(*MemBuffer, ErrorDiagnostics, MainMod->getContext());
        bool broken_debug_info = false;
        if (TmpMod.get() == nullptr ||
            llvm::verifyModule(*TmpMod, &llvm::errs(), &broken_debug_info)) {
          llvm::report_fatal_error("Error: module is broken!");
        }
        if (broken_debug_info) {
          // FIXME at least log this incident
        }
        // now we can safely perform the linking
        if (llvm::Linker::linkModules(*MainMod, std::move(TmpMod),
                                      llvm::Linker::LinkOnlyNeeded)) {
          llvm::report_fatal_error(
              "Error: trying to link modules into single WPA module failed!");
        }
      }
    }
    // Update the IRDB reflecting that we now only need 'MainMod' and its
    // corresponding context!
    // delete every other module
    for (auto it = Modules.begin(); it != Modules.end();) {
      if (it->second.get() != MainMod) {
        it = Modules.erase(it);
      } else {
        ++it;
      }
    }
    // delete every other context
    for (auto it = Contexts.begin(); it != Contexts.end();) {
      if (it->get() != &MainMod->getContext()) {
        it = Contexts.erase(it);
      } else {
        ++it;
      }
    }
    WPAModule = MainMod;
  } else if (Modules.size() == 1) {
    // In this case we only have one module anyway, so we do not have
    // to link at all. But we have to update the WPAMOD pointer!
    WPAModule = Modules.begin()->second.get();
  }
}

void ProjectIRDB::preprocessAllModules() {
  for (auto &[File, Module] : Modules) {
    preprocessModule(Module.get());
  }
}

llvm::Module *ProjectIRDB::getWPAModule() {
  if (!WPAModule) {
    linkForWPA();
  }
  return WPAModule;
}

void ProjectIRDB::buildIDModuleMapping(llvm::Module *M) {
  for (auto &F : *M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        IDInstructionMapping[stol(getMetaDataID(&I))] = &I;
      }
    }
  }
}

bool ProjectIRDB::containsSourceFile(const std::string &File) const {
  return Modules.find(File) != Modules.end();
}

llvm::Module *ProjectIRDB::getModule(const std::string &ModuleName) {
  if (Modules.count(ModuleName))
    return Modules[ModuleName].get();
  return nullptr;
}

std::size_t ProjectIRDB::getNumberOfModules() const { return Modules.size(); }

llvm::Instruction *ProjectIRDB::getInstruction(std::size_t id) {
  if (IDInstructionMapping.count(id))
    return IDInstructionMapping[id];
  return nullptr;
}

std::size_t ProjectIRDB::getInstructionID(const llvm::Instruction *I) const {
  std::size_t id = 0;
  if (auto MD = llvm::cast<llvm::MDString>(
          I->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))) {
    id = stol(MD->getString().str());
  }
  return id;
}

void ProjectIRDB::print() const {
  for (auto &[File, Module] : Modules) {
    std::cout << "Module: " << File << std::endl;
    llvm::outs() << *Module;
  }
}

void ProjectIRDB::emitPreprocessedIR(std::ostream &os, bool shortenIR) const {
  for (auto &[File, Module] : Modules) {
    os << "IR module: " << File << '\n';
    // print globals
    for (auto &glob : Module->globals()) {
      if (shortenIR) {
        os << llvmIRToShortString(&glob);
      } else {
        os << llvmIRToString(&glob);
      }
      os << '\n';
    }
    os << '\n';
    for (auto F : getAllFunctions()) {
      if (!F->isDeclaration() && Module->getFunction(F->getName())) {
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

std::set<const llvm::Instruction *>
ProjectIRDB::getRetOrResInstructions() const {
  return RetOrResInstructions;
}

const llvm::Function *
ProjectIRDB::getFunctionDefinition(const string &FunctionName) const {
  for (auto &[File, Module] : Modules) {
    auto F = Module->getFunction(FunctionName);
    if (F && !F->isDeclaration()) {
      return F;
    }
  }
  return nullptr;
}

const llvm::Function *
ProjectIRDB::getFunction(const std::string &FunctionName) const {
  for (auto &[File, Module] : Modules) {
    auto F = Module->getFunction(FunctionName);
    if (F) {
      return F;
    }
  }
  return nullptr;
}

const llvm::GlobalVariable *ProjectIRDB::getGlobalVariableDefinition(
    const std::string &GlobalVariableName) const {
  for (auto &[File, Module] : Modules) {
    auto G = Module->getGlobalVariable(GlobalVariableName);
    if (G && !G->isDeclaration()) {
      return G;
    }
  }
  return nullptr;
}

llvm::Module *
ProjectIRDB::getModuleDefiningFunction(const std::string &FunctionName) {
  for (auto &[File, Module] : Modules) {
    auto F = Module->getFunction(FunctionName);
    if (F && !F->isDeclaration()) {
      return Module.get();
    }
  }
  return nullptr;
}

const llvm::Module *
ProjectIRDB::getModuleDefiningFunction(const std::string &FunctionName) const {
  for (auto &[File, Module] : Modules) {
    auto F = Module->getFunction(FunctionName);
    if (F && !F->isDeclaration()) {
      return Module.get();
    }
  }
  return nullptr;
}

std::string ProjectIRDB::valueToPersistedString(const llvm::Value *V) {
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(V)) {
    return LLVMZeroValue::getInstance()->getName();
  } else if (const llvm::Instruction *I =
                 llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str() + "." + getMetaDataID(I);
  } else if (const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(V)) {
    return A->getParent()->getName().str() + ".f" +
           std::to_string(A->getArgNo());
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
                   ".o." + std::to_string(idx);
          }
        }
      }
    }
    llvm::report_fatal_error("Error: llvm::Value is of unexpected type.");
    return "";
  } else {
    llvm::report_fatal_error("Error: llvm::Value is of unexpected type.");
    return "";
  }
}

const llvm::Value *ProjectIRDB::persistedStringToValue(const std::string &S) {
  if (S.find(LLVMZeroValue::getInstance()->getName()) != std::string::npos) {
    return LLVMZeroValue::getInstance();
  } else if (S.find(".") == std::string::npos) {
    return getGlobalVariableDefinition(S);
  } else if (S.find(".f") != std::string::npos) {
    unsigned argno = stoi(S.substr(S.find(".f") + 2, S.size()));
    return getNthFunctionArgument(
        getFunctionDefinition(S.substr(0, S.find(".f"))), argno);
  } else if (S.find(".o.") != std::string::npos) {
    unsigned i = S.find(".");
    unsigned j = S.find(".o.");
    unsigned instID = stoi(S.substr(i + 1, j));
    // std::cout << "FOUND instID: " << instID << "\n";
    unsigned opIdx = stoi(S.substr(j + 3, S.size()));
    // std::cout << "FOUND opIdx: " << to_string(opIdx) << "\n";
    const llvm::Function *F = getFunctionDefinition(S.substr(0, S.find(".")));
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (getMetaDataID(&I) == std::to_string(instID)) {
          return I.getOperand(opIdx);
        }
      }
    }
    llvm::report_fatal_error("Error: operand not found.");
  } else if (S.find(".") != std::string::npos) {
    const llvm::Function *F = getFunctionDefinition(S.substr(0, S.find(".")));
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (getMetaDataID(&I) == S.substr(S.find(".") + 1, S.size())) {
          return &I;
        }
      }
    }
    llvm::report_fatal_error("Error: llvm::Instruction not found.");
  } else {
    llvm::report_fatal_error(
        "Error: string cannot be translated into llvm::Value.");
  }
  return nullptr;
}

std::set<const llvm::Instruction *> ProjectIRDB::getAllocaInstructions() const {
  return AllocaInstructions;
}

std::set<const llvm::Function *> ProjectIRDB::getAllFunctions() const {
  std::set<const llvm::Function *> Functions;
  for (auto &[File, Module] : Modules) {
    for (auto &F : *Module) {
      Functions.insert(&F);
    }
  }
  return Functions;
}

bool ProjectIRDB::empty() const { return Modules.empty(); }

void ProjectIRDB::insertModule(llvm::Module *M) {
  Contexts.push_back(std::unique_ptr<llvm::LLVMContext>(&M->getContext()));
  Modules.insert(std::make_pair(M->getModuleIdentifier(), std::move(M)));
  preprocessModule(M);
}

set<const llvm::Type *> ProjectIRDB::getAllocatedTypes() const {
  return AllocatedTypes;
}

std::set<const llvm::StructType *>
ProjectIRDB::getAllocatedStructTypes() const {
  std::set<const llvm::StructType *> StructTypes;
  for (auto Ty : AllocatedTypes) {
    if (auto StructTy = llvm::dyn_cast<llvm::StructType>(Ty)) {
      StructTypes.insert(StructTy);
    }
  }
  return StructTypes;
}

set<const llvm::Value *> ProjectIRDB::getAllMemoryLocations() const {
  // get all stack and heap alloca instructions
  auto AllocaInsts = getAllocaInstructions();
  set<const llvm::Value *> allMemoryLoc;
  for (auto AllocaInst : AllocaInsts) {
    allMemoryLoc.insert(static_cast<const llvm::Value *>(AllocaInst));
  }
  set<string> IgnoredGlobalNames = {"llvm.used",
                                    "llvm.compiler.used",
                                    "llvm.global_ctors",
                                    "llvm.global_dtors",
                                    "vtable",
                                    "typeinfo"};
  // add global varibales to the memory location set, except the llvm
  // intrinsic global variables
  for (auto &[File, Module] : Modules) {
    for (auto &GV : Module->globals()) {
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

bool ProjectIRDB::wasCompiledWithDebugInfo(llvm::Module *M) const {
  return M->getNamedMetadata("llvm.dbg.cu") != nullptr;
}

bool ProjectIRDB::debugInfoAvailable() const {
  if (WPAModule) {
    return wasCompiledWithDebugInfo(WPAModule);
  }
  // During unittests WPAMOD might not be set
  else if (Modules.size() >= 1) {
    for (auto &[File, Module] : Modules) {
      if (!wasCompiledWithDebugInfo(Module.get())) {
        return false;
      }
    }
    return true;
  }
  return false;
}

} // namespace psr
