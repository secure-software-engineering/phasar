/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include "boost/algorithm/string/trim.hpp"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "phasar/Utils/LLVMIRToSrc.h"

using namespace psr;

namespace psr {

llvm::DbgVariableIntrinsic *getDbgVarIntrinsic(const llvm::Value *V) {
  if (auto *VAM = llvm::ValueAsMetadata::getIfExists(
          const_cast<llvm::Value *>(V))) { // NOLINT FIXME when LLVM supports it
    if (auto *MDV = llvm::MetadataAsValue::getIfExists(V->getContext(), VAM)) {
      for (auto *U : MDV->users()) {
        if (auto *DBGIntr = llvm::dyn_cast<llvm::DbgVariableIntrinsic>(U)) {
          return DBGIntr;
        }
      }
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    /* If mem2reg is not activated, formal parameters will be stored in
     * registers at the beginning of function call. Debug info will be linked to
     * those alloca's instead of the arguments itself. */
    for (const auto *User : Arg->users()) {
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
        if (Store->getValueOperand() == Arg &&
            llvm::isa<llvm::AllocaInst>(Store->getPointerOperand())) {
          return getDbgVarIntrinsic(Store->getPointerOperand());
        }
      }
    }
  }
  return nullptr;
}

llvm::DILocalVariable *getDILocalVariable(const llvm::Value *V) {
  if (auto *DbgIntr = getDbgVarIntrinsic(V)) {
    if (auto *DDI = llvm::dyn_cast<llvm::DbgDeclareInst>(DbgIntr)) {
      return DDI->getVariable();
    }
    if (auto *DVI = llvm::dyn_cast<llvm::DbgValueInst>(DbgIntr)) {
      return DVI->getVariable();
    }
  }
  return nullptr;
}

llvm::DIGlobalVariable *getDIGlobalVariable(const llvm::Value *V) {
  if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (auto *MN = GV->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto *DIGVExp =
              llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return DIGVExp->getVariable();
      }
    }
  }
  return nullptr;
}

llvm::DISubprogram *getDISubprogram(const llvm::Value *V) {
  if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getSubprogram();
  }
  return nullptr;
}

llvm::DILocation *getDILocation(const llvm::Value *V) {
  // Arguments and Instruction such as AllocaInst
  if (auto *DbgIntr = getDbgVarIntrinsic(V)) {
    if (auto *MN = DbgIntr->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  } else if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (auto *MN = I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  }
  return nullptr;
}

llvm::DIFile *getDIFile(const llvm::Value *V) {
  if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    if (auto *MN = GO->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto *Subpr = llvm::dyn_cast<llvm::DISubprogram>(MN)) {
        return Subpr->getFile();
      }
      if (auto *GVExpr = llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return GVExpr->getVariable()->getFile();
      }
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    if (auto *LocVar = getDILocalVariable(Arg)) {
      return LocVar->getFile();
    }
  } else if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (I->isUsedByMetadata()) {
      if (auto *LocVar = getDILocalVariable(I)) {
        return LocVar->getFile();
      }
    } else if (I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return I->getDebugLoc()->getFile();
    }
  }
  return nullptr;
}

std::string getVarNameFromIR(const llvm::Value *V) {
  if (auto *LocVar = getDILocalVariable(V)) {
    return LocVar->getName().str();
  }
  if (auto *GlobVar = getDIGlobalVariable(V)) {
    return GlobVar->getName().str();
  }
  return "";
}

std::string getFunctionNameFromIR(const llvm::Value *V) {
  // We can return unmangled function names w/o checking debug info
  if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getName().str();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getName().str();
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str();
  }
  return "";
}

std::string getFilePathFromIR(const llvm::Value *V) {
  if (auto *DIF = getDIFile(V)) {
    std::filesystem::path File(DIF->getFilename().str());
    std::filesystem::path Dir(DIF->getDirectory().str());
    if (!File.empty()) {
      // try to concatenate file path and dir to get absolut path
      if (!File.has_root_directory() && !Dir.empty()) {
        File = Dir / File;
      }
      return File.string();
    }
  } else {
    /* As a fallback solution, we will return 'source_filename' info from
     * module. However, it is not guaranteed to contain the absoult path, and it
     * will return 'llvm-link' for linked modules. */
    if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
      return F->getParent()->getSourceFileName();
    }
    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
      return Arg->getParent()->getParent()->getSourceFileName();
    }
    if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
      return I->getFunction()->getParent()->getSourceFileName();
    }
  }
  return "";
}

unsigned int getLineFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getLine();
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getLine();
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getLine();
  }
  return 0;
}

std::string getDirectoryFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getDirectory().str();
  }
  if (auto *DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getDirectory().str();
  }
  if (auto *DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getDirectory().str();
  }
  return "";
}

unsigned int getColumnFromIR(const llvm::Value *V) {
  // Globals and Function have no column info
  if (auto *DILoc = getDILocation(V)) {
    return DILoc->getColumn();
  }
  return 0;
}

std::string getSrcCodeFromIR(const llvm::Value *V) {
  unsigned int LineNr = getLineFromIR(V);
  if (LineNr > 0) {
    std::filesystem::path Path(getFilePathFromIR(V));
    if (std::filesystem::exists(Path) && !std::filesystem::is_directory(Path)) {
      std::ifstream Ifs(Path.string(), std::ios::binary);
      if (Ifs.is_open()) {
        Ifs.seekg(std::ios::beg);
        std::string SrcLine;
        for (unsigned int I = 0; I < LineNr - 1; ++I) {
          Ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::getline(Ifs, SrcLine);
        boost::algorithm::trim(SrcLine);
        return SrcLine;
      }
    }
  }
  return "";
}

std::string getModuleIDFromIR(const llvm::Value *V) {
  if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    return GO->getParent()->getModuleIdentifier();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getParent()->getModuleIdentifier();
  }
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getParent()->getModuleIdentifier();
  }
  return "";
}

bool SourceCodeInfo::empty() const noexcept { return SourceCodeLine.empty(); }

bool SourceCodeInfo::operator==(const SourceCodeInfo &Other) const noexcept {
  // don't compare the SourceCodeFunctionName. It is directly derivable from
  // line, column and filename
  return Line == Other.Line && Column == Other.Column &&
         SourceCodeLine == Other.SourceCodeLine &&
         SourceCodeFilename == Other.SourceCodeFilename;
}

bool SourceCodeInfo::equivalentWith(const SourceCodeInfo &Other) const {
  // Here, we need to compare the SourceCodeFunctionName, because we don't
  // compare the complete SourceCodeFilename
  if (Line != Other.Line || Column != Other.Column ||
      SourceCodeLine != Other.SourceCodeLine ||
      SourceCodeFunctionName != Other.SourceCodeFunctionName) {
    return false;
  }

  auto Pos = SourceCodeFilename.find_last_of(
      std::filesystem::path::preferred_separator);
  if (Pos == std::string::npos) {
    Pos = 0;
  }

  return llvm::StringRef(Other.SourceCodeFilename)
      .endswith(llvm::StringRef(SourceCodeFilename)
                    .slice(Pos + 1, llvm::StringRef::npos));
}

void from_json(const nlohmann::json &J, SourceCodeInfo &Info) {
  J.at("sourceCodeLine").get_to(Info.SourceCodeLine);
  J.at("sourceCodeFileName").get_to(Info.SourceCodeFilename);
  if (auto Fn = J.find("sourceCode"); Fn != J.end()) {
    Fn->get_to(Info.SourceCodeFunctionName);
  }
  J.at("line").get_to(Info.Line);
  J.at("column").get_to(Info.Column);
}
void to_json(nlohmann::json &J, const SourceCodeInfo &Info) {
  J = nlohmann::json{
      {"sourceCodeLine", Info.SourceCodeLine},
      {"sourceCodeFileName", Info.SourceCodeFilename},
      {"sourceCodeFunctionName", Info.SourceCodeFunctionName},
      {"line", Info.Line},
      {"column", Info.Column},
  };
}

SourceCodeInfo getSrcCodeInfoFromIR(const llvm::Value *V) {
  return SourceCodeInfo{
      getSrcCodeFromIR(V),
      getFilePathFromIR(V),
      llvm::demangle(getFunctionNameFromIR(V)),
      getLineFromIR(V),
      getColumnFromIR(V),
  };
}

} // namespace psr
