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

#include "phasar/Config/phasar-config.h"
#include "phasar/Utils/ErrorHandling.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include <cstdlib>
#include <iterator>
#include <system_error>

using namespace psr;

namespace psr {
/// Name of the file storing all glibc function names.
static constexpr llvm::StringLiteral GLIBCFunctionListFileName =
    "glibc_function_list_v1-04.05.17.conf";

/// Name of the file storing all LLVM intrinsic function names.
static constexpr llvm::StringLiteral LLVMIntrinsicFunctionListFileName =
    "llvm_intrinsics_function_list_v1-04.05.17.conf";

llvm::StringRef PhasarConfig::PhasarVersion() noexcept {
  return PHASAR_VERSION_STRING;
}

llvm::StringRef PhasarConfig::GlobalConfigurationDirectory() noexcept {
  return PHASAR_CONFIG_DIR;
}

llvm::StringRef PhasarConfig::PhasarDirectory() noexcept {
  return PHASAR_SRC_DIR;
}

llvm::StringRef PhasarConfig::DefaultSourceSinkFunctionsPath() noexcept {
  return PHASAR_SRC_DIR "/config/phasar-source-sink-function.json";
}

static bool loadConfigFileInto(PhasarConfig &PC, llvm::StringRef FileName,
                               std::set<std::string> &Lines) {
  auto ConfigFile = PC.readConfigFileAsTextOrErr(FileName);
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

  llvm::transform(
      ConfigLines, std::inserter(Lines, Lines.end()), [](llvm::StringRef Str) {
        if (auto Comment = Str.find("//"); Comment != llvm::StringRef::npos) {
          Str = Str.slice(0, Comment);
        }
        return Str.trim().str();
      });
  return true;
}

static void loadGlibcSpecialFunctionNames(PhasarConfig &PC,
                                          std::set<std::string> &Into) {
  if (!loadConfigFileInto(PC, GLIBCFunctionListFileName, Into)) {
    // Add default glibc function names
    Into.insert({"_exit"});
  }
}

static void loadLLVMSpecialFunctionNames(PhasarConfig &PC,
                                         std::set<std::string> &Into) {
  if (!loadConfigFileInto(PC, LLVMIntrinsicFunctionListFileName, Into)) {
    // Add default LLVM function names
    Into.insert({"llvm.va_start"});
  }
}

PhasarConfig::PhasarConfig() {
  loadGlibcSpecialFunctionNames(*this, SpecialFuncNames);
  loadLLVMSpecialFunctionNames(*this, SpecialFuncNames);

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

PhasarConfig &PhasarConfig::getPhasarConfig() {
  static PhasarConfig PC{};
  return PC;
}

} // namespace psr
