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

#include "phasar/Config/Configuration.h"

#include "phasar/Utils/ErrorHandling.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <system_error>

using namespace psr;

namespace psr {
llvm::StringRef PhasarConfig::PhasarVersion() noexcept {
  return XSTR(PHASAR_VERSION);
}

llvm::StringRef PhasarConfig::GlobalConfigurationDirectory() noexcept {
  return PHASAR_CONFIG_DIR;
}

llvm::StringRef PhasarConfig::PhasarDirectory() noexcept { return PHASAR_DIR; }

llvm::StringRef PhasarConfig::DefaultSourceSinkFunctionsPath() noexcept {
  return PHASAR_DIR "/config/phasar-source-sink-function.json";
}

PhasarConfig::PhasarConfig() {
  loadGlibcSpecialFunctionNames();
  loadLLVMSpecialFunctionNames();

  // Insert allocation operators
  SpecialFuncNames.insert({"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"});
}

std::optional<llvm::StringRef>
PhasarConfig::LocalConfigurationDirectory() noexcept {
  static std::string DirName = []() -> std::string {
    llvm::SmallString<256> HomePath;
    if (llvm::sys::path::home_directory(HomePath)) {
      return (HomePath + "/.config/phasar/").str();
    }

    return {};
  }();
  if (DirName.empty()) {
    return std::nullopt;
  }
  return DirName;
}

std::unique_ptr<llvm::MemoryBuffer>
PhasarConfig::readConfigFile(const llvm::Twine &FileName) {
  return getOrThrow(readConfigFileOrErr(FileName));
}

std::string PhasarConfig::readConfigFileAsText(const llvm::Twine &FileName) {
  auto Buffer = readConfigFile(FileName);
  return Buffer->getBuffer().str();
}

llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
PhasarConfig::readConfigFileOrErr(const llvm::Twine &FileName) {
  if (auto LocalConfigPath = LocalConfigurationDirectory()) {
    if (llvm::sys::fs::exists(*LocalConfigPath + FileName)) {
      PHASAR_LOG_LEVEL(INFO,
                       "Local config file: " << (*LocalConfigPath + FileName));
      return readFileOrErr(*LocalConfigPath + FileName);
    }
  }
  PHASAR_LOG_LEVEL(INFO, "Global config file: "
                             << (GlobalConfigurationDirectory() + FileName));
  return readFileOrErr(GlobalConfigurationDirectory() + FileName);
}
llvm::ErrorOr<std::string>
PhasarConfig::readConfigFileAsTextOrErr(const llvm::Twine &FileName) {
  return mapValue(readConfigFileOrErr(FileName),
                  [](auto Buffer) { return Buffer->getBuffer().str(); });
}

std::unique_ptr<llvm::MemoryBuffer>
PhasarConfig::readConfigFileOrNull(const llvm::Twine &FileName) {
  return getOrEmpty(readConfigFileOrErr(FileName));
}
std::optional<std::string>
PhasarConfig::readConfigFileAsTextOrNull(const llvm::Twine &FileName) {
  if (auto Buffer = readConfigFileOrNull(FileName)) {
    return Buffer->getBuffer().str();
  }
  return std::nullopt;
}

bool PhasarConfig::loadConfigFileInto(llvm::StringRef FileName,
                                      std::set<std::string> &Lines) {
  auto ConfigFile = readConfigFileAsTextOrErr(FileName);
  if (!ConfigFile) {
    if (ConfigFile.getError() != std::errc::no_such_file_or_directory) {
      PHASAR_LOG_LEVEL(WARNING, "Could not open config file '"
                                    << FileName << "': "
                                    << ConfigFile.getError().message());
    }

    return false;
  }

  llvm::SmallVector<llvm::StringRef, 0> ConfigLines;
  llvm::SplitString(*ConfigFile, ConfigLines, "\n");

  llvm::transform(ConfigLines, std::inserter(Lines, Lines.end()),
                  [](llvm::StringRef Str) { return Str.trim().str(); });
  return true;
}

void PhasarConfig::loadGlibcSpecialFunctionNames() {
  if (!loadConfigFileInto(GLIBCFunctionListFileName, SpecialFuncNames)) {
    // Add default glibc function names
    SpecialFuncNames.insert({"_exit"});
  }
}

void PhasarConfig::loadLLVMSpecialFunctionNames() {
  if (!loadConfigFileInto(LLVMIntrinsicFunctionListFileName,
                          SpecialFuncNames)) {
    // Add default LLVM function names
    SpecialFuncNames.insert({"llvm.va_start"});
  }
}

PhasarConfig &PhasarConfig::getPhasarConfig() {
  static PhasarConfig PC{};
  return PC;
}

} // namespace psr
