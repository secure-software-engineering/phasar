/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Configuration.cpp
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#include <fstream>

#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/split.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/Config/Version.h"
#include "phasar/Utils/IO.h"

using namespace psr;

namespace psr {

PhasarConfig::PhasarConfig() {
  loadGlibcSpecialFunctionNames();
  loadLLVMSpecialFunctionNames();

  // Insert allocation operators
  SpecialFuncNames.insert({"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"});
}

const std::string &PhasarConfig::ConfigurationDirectory() {
  static const std::string ConfigDir = [] {
    auto *EnvHome = std::getenv("HOME");
    std::string ConfigFolder = "config/";
    if (EnvHome) { // Check if HOME was defined in the environment
      std::string PhasarConfDir = std::string(EnvHome) + "/.config/phasar/";
      if (std::filesystem::exists(PhasarConfDir) &&
          std::filesystem::is_directory(PhasarConfDir)) {
        ConfigFolder = PhasarConfDir;
      }
    }
    return ConfigFolder;
  }();

  return ConfigDir;
}

/// Specifies the directory in which Phasar is located.
const std::string &PhasarConfig::PhasarDirectory() {
  static const std::string PhasarDir = [] {
    std::string CurrPath = std::filesystem::current_path().string();
    size_t I = CurrPath.rfind("build", CurrPath.length());
    return CurrPath.substr(0, I);
  }();

  return PhasarDir;
}

void PhasarConfig::loadGlibcSpecialFunctionNames() {
  const std::string GLIBCFunctionListFilePath =
      ConfigurationDirectory() + GLIBCFunctionListFileName;

  if (std::filesystem::exists(GLIBCFunctionListFilePath)) {
    // Load glibc function names specified in the config file
    std::vector<std::string> GlibcFunctions;
    std::string Glibc = readTextFile(GLIBCFunctionListFilePath);
    // Insert glibc function names
    boost::split(GlibcFunctions, Glibc, boost::is_any_of("\n"),
                 boost::token_compress_on);

    SpecialFuncNames.insert(GlibcFunctions.begin(), GlibcFunctions.end());
  } else {
    // Add default glibc function names
    SpecialFuncNames.insert({"_exit"});
  }
}

void PhasarConfig::loadLLVMSpecialFunctionNames() {
  const std::string LLVMFunctionListFilePath =
      ConfigurationDirectory() + LLVMIntrinsicFunctionListFileName;
  if (std::filesystem::exists(LLVMFunctionListFilePath)) {
    // Load LLVM function names specified in the config file
    std::string LLVMIntrinsics = readTextFile(LLVMFunctionListFilePath);

    std::vector<std::string> LLVMIntrinsicFunctions;
    boost::split(LLVMIntrinsicFunctions, LLVMIntrinsics, boost::is_any_of("\n"),
                 boost::token_compress_on);

    // Insert llvm intrinsic function names
    SpecialFuncNames.insert(LLVMIntrinsicFunctions.begin(),
                            LLVMIntrinsicFunctions.end());
  } else {
    // Add default LLVM function names
    SpecialFuncNames.insert({"llvm.va_start"});
  }
}

PhasarConfig &PhasarConfig::getPhasarConfig() {
  static PhasarConfig PC;
  return PC;
}

} // namespace psr
