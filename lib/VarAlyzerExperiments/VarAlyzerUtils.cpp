/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"

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
  if (Str == "MD") {
    return OpenSSLEVPAnalysisType::MD;
  }
  if (Str == "CIPHER") {
    return OpenSSLEVPAnalysisType::CIPHER;
  }
  if (Str == "MAC") {
    return OpenSSLEVPAnalysisType::MAC;
  }
  if (Str == "ALL") {
    return OpenSSLEVPAnalysisType::ALL;
  }
  return OpenSSLEVPAnalysisType::ALL;
}

} // namespace psr
