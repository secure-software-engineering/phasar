/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <fstream>
#include <iostream>
#include <limits>

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <phasar/Utils/LLVMIRToSrc.h>

using namespace psr;

namespace psr {

std::string getSrcCodeLine(const std::string &Dir, const std::string &File,
                           unsigned int num) {
  boost::filesystem::path FilePath;
  // Its possible that File holds the complete path
  if (File.find("/home/") == std::string::npos) {
    boost::filesystem::path DirPath(Dir);
    boost::filesystem::path FileName(File);
    FilePath = DirPath / FileName;
  } else {
    FilePath = File;
  }
  if (boost::filesystem::exists(FilePath) &&
      !boost::filesystem::is_directory(FilePath)) {
    std::ifstream ifs(FilePath.string(), std::ios::binary);
    if (ifs.is_open()) {
      ifs.seekg(std::ios::beg);
      std::string line;
      for (unsigned int i = 0; i < num - 1; ++i) {
        ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      std::getline(ifs, line);
      boost::algorithm::trim(line);
      return line;
    }
  }
  throw std::ios_base::failure("could not read file: " + FilePath.string());
}

llvm::DILocalVariable *getDILocVarFromValue(const llvm::Value *V) {
  if (auto *L =
          llvm::LocalAsMetadata::getIfExists(const_cast<llvm::Value *>(V))) {
    if (auto *MDV = llvm::MetadataAsValue::getIfExists(V->getContext(), L)) {
      for (auto *U : MDV->users()) {
        if (auto *DDI = llvm::dyn_cast<llvm::DbgDeclareInst>(U)) {
          return DDI->getVariable();
        }
        if (auto *DVI = llvm::dyn_cast<llvm::DbgValueInst>(U)) {
          return DVI->getVariable();
        }
      }
    }
  }
  return nullptr;
}

std::string getLocalVarSrcInfo(const llvm::Value *V, bool ScopeInfo) {
  if (auto DILocVar = getDILocVarFromValue(V)) {
    std::string VarName("Var : " + DILocVar->getName().str() + "\n");
    std::string Line("Line: " + std::to_string(DILocVar->getLine()));
    std::string Scope;
    if (ScopeInfo) {
      Scope = "\nFunc: " +
              DILocVar->getScope()->getSubprogram()->getName().str() + "\n";
      Scope += "File: " + DILocVar->getScope()->getFilename().str();
    }
    return VarName + Line + Scope;
  }
  return "No source information available!";
}

std::string llvmInstructionToSrc(const llvm::Instruction *I, bool ScopeInfo) {
  // Get source variable name if available
  if (I->isUsedByMetadata()) {
    return getLocalVarSrcInfo(I, ScopeInfo);
  }
  // Otherwise get the corresponding source code line instead
  if (I->getMetadata(llvm::LLVMContext::MD_dbg)) {
    auto &DILoc = I->getDebugLoc();
    std::string ScopeStr;
    std::string SrcCode("Src : ");
    if (auto Scope = DILoc->getScope()) {
      if (ScopeInfo) {
        // Scope is not necessarily a DISubprogram - could be a DILexicalblock
        ScopeStr = "Func: " + Scope->getSubprogram()->getName().str() + "\n";
        ScopeStr += "File: " + Scope->getFilename().str() + "\n";
      }
      SrcCode += getSrcCodeLine(Scope->getDirectory().str(),
                                Scope->getFilename().str(), DILoc.getLine());
      SrcCode += "\n";
    } else {
      SrcCode += "No source code found!\n";
    }
    std::string Line = "Line: " + std::to_string(DILoc.getLine()) + "\n";
    std::string Col = "Col : " + std::to_string(DILoc.getCol());
    return SrcCode + ScopeStr + Line + Col;
  }
  return "No source information available!";
}

std::string llvmArgumentToSrc(const llvm::Argument *A, bool ScopeInfo) {
  return getLocalVarSrcInfo(A, ScopeInfo);
}

std::string llvmFunctionToSrc(const llvm::Function *F) {
  if (auto SubProg = F->getSubprogram()) {
    return "Fname: " + SubProg->getName().str() +
           "\nLine : " + std::to_string(SubProg->getLine()) +
           "\nFile : " + SubProg->getFilename().str();
    ;
  } else {
    return "Fname: " + F->getName().str();
  }
}

std::string llvmGlobalValueToSrc(const llvm::GlobalVariable *GV) {
  // There is no dbg info for external global variable's
  if (auto DbgMetaNode = GV->getMetadata(llvm::LLVMContext::MD_dbg)) {
    if (auto DIGVExp =
            llvm::dyn_cast<llvm::DIGlobalVariableExpression>(DbgMetaNode)) {
      if (auto DIGV = DIGVExp->getVariable()) {
        return "Var : " + DIGV->getName().str() +
               "\nLine: " + std::to_string(DIGV->getLine()) +
               "\nFile: " + DIGV->getFilename().str();
      }
    }
  }
  return "No source information available!";
}

std::string llvmModuleToSrc(const llvm::Module *M) {
  return M->getModuleIdentifier();
}

std::string llvmValueToSrc(const llvm::Value *V, bool ScopeInfo) {
  // if possible delegate call to appropriate method
  if (auto F = llvm::dyn_cast<llvm::Function>(V)) {
    return llvmFunctionToSrc(F);
  }
  if (auto GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    return llvmGlobalValueToSrc(GV);
  }
  if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return llvmArgumentToSrc(Arg, ScopeInfo);
  }
  if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return llvmInstructionToSrc(I, ScopeInfo);
  }
  return "No source information available!";
}

} // namespace psr
