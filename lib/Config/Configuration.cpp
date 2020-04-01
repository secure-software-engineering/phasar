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
#include "boost/filesystem.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/Config/Version.h"

using namespace psr;

namespace psr {

PhasarConfig::PhasarConfig() {
  loadGlibcSpecialFunctionNames();
  loadLLVMSpecialFunctionNames();

  // Insert allocation operators
  special_function_names.insert({"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"});
}

std::string PhasarConfig::readConfigFile(const std::string &path) {
  // We use a local file reading function to make phasar_config independent of
  // other phasar libraries.
  if (boost::filesystem::exists(path) &&
      !boost::filesystem::is_directory(path)) {
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.is_open()) {
      ifs.seekg(0, ifs.end);
      size_t file_size = ifs.tellg();
      ifs.seekg(0, ifs.beg);
      std::string content(file_size + 1, '\0');
      ifs.read(const_cast<char *>(content.data()), file_size);
      return content;
    }
  }
  throw std::ios_base::failure("could not read file: " + path);
}

void PhasarConfig::loadGlibcSpecialFunctionNames() {
  const std::string GLIBCFunctionListFilePath =
      ConfigurationDirectory() + GLIBCFunctionListFileName;

  if (boost::filesystem::exists(GLIBCFunctionListFilePath)) {
    // Load glibc function names specified in the config file
    std::vector<std::string> glibcfunctions;
    std::string glibc = readConfigFile(GLIBCFunctionListFilePath);
    // Insert glibc function names
    boost::split(glibcfunctions, glibc, boost::is_any_of("\n"),
                 boost::token_compress_on);

    special_function_names.insert(glibcfunctions.begin(), glibcfunctions.end());
  } else {
    // Add default glibc function names
    special_function_names.insert({"_exit"});
  }
}

void PhasarConfig::loadLLVMSpecialFunctionNames() {
  const std::string LLVMFunctionListFilePath =
      ConfigurationDirectory() + LLVMIntrinsicFunctionListFileName;
  if (boost::filesystem::exists(LLVMFunctionListFilePath)) {
    // Load LLVM function names specified in the config file
    std::string llvmintrinsics = readConfigFile(LLVMFunctionListFilePath);

    std::vector<std::string> llvmintrinsicfunctions;
    boost::split(llvmintrinsicfunctions, llvmintrinsics, boost::is_any_of("\n"),
                 boost::token_compress_on);

    // Insert llvm intrinsic function names
    special_function_names.insert(llvmintrinsicfunctions.begin(),
                                  llvmintrinsicfunctions.end());
  } else {
    // Add default LLVM function names
    special_function_names.insert({"llvm.va_start"});
  }
}

const std::string PhasarConfig::phasar_directory = std::string([]() {
  std::string curr_path = boost::filesystem::current_path().string();
  size_t i = curr_path.rfind("build", curr_path.length());
  return curr_path.substr(0, i);
}());

PhasarConfig &PhasarConfig::getPhasarConfig() {
  static PhasarConfig PC;
  return PC;
}

} // namespace psr
