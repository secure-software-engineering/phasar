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

llvm::DbgVariableIntrinsic *getDbgVarIntrinsic(const llvm::Value *V) {
  if (auto *VAM =
          llvm::ValueAsMetadata::getIfExists(const_cast<llvm::Value *>(V))) {
    if (auto *MDV = llvm::MetadataAsValue::getIfExists(V->getContext(), VAM)) {
      for (auto *U : MDV->users()) {
        if (auto DBGIntr = llvm::dyn_cast<llvm::DbgVariableIntrinsic>(U)) {
          return DBGIntr;
        }
      }
    }
  } else if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    /* If mem2reg is not activated, formal parameters will be stored in
     * registers at the beginning of function call. Debug info will be linked to
     * those alloca's instead of the arguments itself. */
    for (auto User : Arg->users()) {
      if (auto Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
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
  if (auto DbgIntr = getDbgVarIntrinsic(V)) {
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
  if (auto GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (auto MN = GV->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto DIGVExp = llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return DIGVExp->getVariable();
      }
    }
  }
  return nullptr;
}

llvm::DISubprogram *getDISubprogram(const llvm::Value *V) {
  if (auto F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getSubprogram();
  }
  return nullptr;
}

llvm::DILocation *getDILocation(const llvm::Value *V) {
  // Arguments and Instruction such as AllocaInst
  if (auto DbgIntr = getDbgVarIntrinsic(V)) {
    if (auto MN = DbgIntr->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  } else if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (auto MN = I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return llvm::dyn_cast<llvm::DILocation>(MN);
    }
  }
  return nullptr;
}

llvm::DIFile *getDIFile(const llvm::Value *V) {
  if (auto GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    if (auto MN = GO->getMetadata(llvm::LLVMContext::MD_dbg)) {
      if (auto Subpr = llvm::dyn_cast<llvm::DISubprogram>(MN)) {
        return Subpr->getFile();
      }
      if (auto GVExpr = llvm::dyn_cast<llvm::DIGlobalVariableExpression>(MN)) {
        return GVExpr->getVariable()->getFile();
      }
    }
  } else if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    if (auto LocVar = getDILocalVariable(Arg)) {
      return LocVar->getFile();
    }
  } else if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (I->isUsedByMetadata()) {
      if (auto LocVar = getDILocalVariable(I)) {
        return LocVar->getFile();
      }
    } else if (I->getMetadata(llvm::LLVMContext::MD_dbg)) {
      return I->getDebugLoc()->getFile();
    }
  }
  return nullptr;
}

std::string getVarNameFromIR(const llvm::Value *V) {
  if (auto LocVar = getDILocalVariable(V)) {
    return LocVar->getName().str();
  } else if (auto GlobVar = getDIGlobalVariable(V)) {
    return GlobVar->getName().str();
  }
  return "";
}

std::string getFunctionNameFromIR(const llvm::Value *V) {
  // We can return unmangled function names w/o checking debug info
  if (auto F = llvm::dyn_cast<llvm::Function>(V)) {
    return F->getName().str();
  } else if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getName().str();
  } else if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getName().str();
  }
  return "";
}

std::string getFilePathFromIR(const llvm::Value *V) {
  if (auto DIF = getDIFile(V)) {
    boost::filesystem::path file(DIF->getFilename().str());
    boost::filesystem::path dir(DIF->getDirectory().str());
    if (!file.empty()) {
      // try to concatenate file path and dir to get absolut path
      if (!file.has_root_directory() && !dir.empty()) {
        file = dir / file;
      }
      return file.string();
    }
  } else {
    /* As a fallback solution, we will return 'source_filename' info from
     * module. However, it is not guaranteed to contain the absoult path, and it
     * will return 'llvm-link' for linked modules. */
    if (auto F = llvm::dyn_cast<llvm::Function>(V)) {
      return F->getParent()->getSourceFileName();
    } else if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
      return Arg->getParent()->getParent()->getSourceFileName();
    } else if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
      return I->getFunction()->getParent()->getSourceFileName();
    }
  }
  return "";
}

unsigned int getLineFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto DILoc = getDILocation(V)) {
    return DILoc->getLine();
  } else if (auto DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getLine();
  } else if (auto DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getLine();
  }
  return 0;
}

std::string getDirectoryFromIR(const llvm::Value *V) {
  // Argument and Instruction
  if (auto DILoc = getDILocation(V)) {
    return DILoc->getDirectory();
  } else if (auto DISubpr = getDISubprogram(V)) { // Function
    return DISubpr->getDirectory();
  } else if (auto DIGV = getDIGlobalVariable(V)) { // Globals
    return DIGV->getDirectory();
  }
  return 0;
}

unsigned int getColumnFromIR(const llvm::Value *V) {
  // Globals and Function have no column info
  if (auto DILoc = getDILocation(V)) {
    return DILoc->getColumn();
  }
  return 0;
}

std::string getSrcCodeFromIR(const llvm::Value *V) {
  unsigned int lineNr = getLineFromIR(V);
  if (lineNr > 0) {
    boost::filesystem::path path(getFilePathFromIR(V));
    if (boost::filesystem::exists(path) &&
        !boost::filesystem::is_directory(path)) {
      std::ifstream ifs(path.string(), std::ios::binary);
      if (ifs.is_open()) {
        ifs.seekg(std::ios::beg);
        std::string src_line;
        for (unsigned int i = 0; i < lineNr - 1; ++i) {
          ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::getline(ifs, src_line);
        boost::algorithm::trim(src_line);
        return src_line;
      }
    }
  }
  return "";
}

std::string getModuleIDFromIR(const llvm::Value *V) {
  if (auto GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    return GO->getParent()->getModuleIdentifier();
  } else if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent()->getParent()->getModuleIdentifier();
  } else if (auto I = llvm::dyn_cast<llvm::Instruction>(V)) {
    return I->getFunction()->getParent()->getModuleIdentifier();
  }
  return "";
}

} // namespace psr
