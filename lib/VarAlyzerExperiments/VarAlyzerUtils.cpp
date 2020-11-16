/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"
#include "phasar/Utils/LLVMShorthands.h"
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
                                    LLVMBasedICFG &ICF) {
  auto FNameMap = extractStaticRenaming(&IR);
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

} // namespace psr
