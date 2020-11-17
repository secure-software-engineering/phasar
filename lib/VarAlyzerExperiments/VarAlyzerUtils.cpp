/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/VarAlyzerExperiments/VarAlyzerUtils.h"

using namespace psr;

namespace psr {

bool isValidLLVMIRFile(const boost::filesystem::path &FilePath) {
  return boost::filesystem::exists(FilePath) &&
         !boost::filesystem::is_directory(FilePath) &&
         (FilePath.extension() == ".ll" || FilePath.extension() == ".bc");
}

std::vector<std::string> makeStringVectorFromPathVector(
    const std::vector<boost::filesystem::path> &Paths) {
  std::vector<std::string> Result;
  Result.reserve(Paths.size());
  for (const auto &Path : Paths) {
    Result.push_back(Path.string());
  }
  return Result;
}

OpenSSLEVPAnalysisType to_OpenSSLEVPAnalysisType(const std::string &Str) {
  if (Str == "CIPHER") {
    return OpenSSLEVPAnalysisType::CIPHER;
  }
  if (Str == "MAC") {
    return OpenSSLEVPAnalysisType::MAC;
  }
  if (Str == "MD") {
    return OpenSSLEVPAnalysisType::MD;
  }
  llvm::report_fatal_error("input string is invalid");
}

std::set<std::string> getEntryPointsForCallersOf(const std::string &FunName,
                                                 ProjectIRDB &IR,
                                                 LLVMBasedICFG &ICF) {
  const auto *F = IR.getFunction(FunName);
  auto CallSites = ICF.getCallersOf(F);
  std::set<std::string> EntryPoints;
  for (const auto *CallSite : CallSites) {
    EntryPoints.insert(CallSite->getFunction()->getName().str());
  }
  return EntryPoints;
}

std::set<std::string>
getEntryPointsForCallersOfDesugared(const std::string &FunName, ProjectIRDB &IR,
                                    LLVMBasedICFG &ICF,
                                    const stringstringmap_t &FNameMap) {
  auto Search = FNameMap.find(FunName);
  assert(Search != FNameMap.end() && "Expected to find FunName in FNameMap!");
  auto DesugaredFName = Search->second;
  const auto *F = IR.getFunction(DesugaredFName);
  auto CallSites = ICF.getCallersOf(IR.getFunction(DesugaredFName));
  std::set<std::string> EntryPoints;
  for (const auto *CallSite : CallSites) {
    EntryPoints.insert(CallSite->getFunction()->getName().str());
  }
  return EntryPoints;
}

std::optional<llvm::StringRef>
getBaseTypeNameIfUsingTypeDef(const llvm::AllocaInst *A) {
  const auto *F = A->getFunction();
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (const auto *DbgDeclare = llvm::dyn_cast<llvm::DbgDeclareInst>(&I)) {
        if (DbgDeclare->getAddress() == A) {
          const auto *LocalVar = DbgDeclare->getVariable();
          if (const auto *DerivedTy =
                  llvm::dyn_cast<llvm::DIDerivedType>(LocalVar->getType())) {
            while ((DerivedTy = llvm::dyn_cast<llvm::DIDerivedType>(
                        DerivedTy->getBaseType()))) {
              if (DerivedTy->getTag() == llvm::dwarf::DW_TAG_typedef) {
                return DerivedTy->getName();
              }
            }
          }
        }
      }
    }
  }
  return std::nullopt;
}

llvm::StringRef staticRename(llvm::StringRef Name,
                             const stringstringmap_t &Renaming) {
  if (auto it = Renaming.find(Name); it != Renaming.end())
    return it->getValue();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Renaming fallthrough: " << Name.str());

  return Name;
}

} // namespace psr
