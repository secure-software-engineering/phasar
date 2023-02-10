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

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <system_error>

using namespace psr;

namespace psr {

PhasarConfig::PhasarConfig() {
  loadGlibcSpecialFunctionNames();
  loadLLVMSpecialFunctionNames();

  // Insert allocation operators
  SpecialFuncNames.insert({"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"});
}

std::optional<llvm::StringRef>
PhasarConfig::LocalConfigurationDirectory() noexcept {
  static std::string DirName = []() -> std::string {
    const auto *Home = getenv("HOME");
    if (!Home) {
      return {};
    }
    return (Home + llvm::Twine("/.config/phasar/")).str();
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

void PhasarConfig::loadGlibcSpecialFunctionNames() {
  auto Glibc = readConfigFileAsTextOrErr(GLIBCFunctionListFileName);
  if (!Glibc) {
    if (Glibc.getError() != std::errc::no_such_file_or_directory) {
      PHASAR_LOG_LEVEL(WARNING, "Could not open config file '"
                                    << GLIBCFunctionListFileName
                                    << "': " << Glibc.getError().message());
    }
    // Add default glibc function names
    SpecialFuncNames.insert({"_exit"});
    return;
  }

  llvm::SmallVector<llvm::StringRef, 0> GlibcFunctions;
  llvm::SplitString(*Glibc, GlibcFunctions, "\n");

  llvm::transform(GlibcFunctions,
                  std::inserter(SpecialFuncNames, SpecialFuncNames.end()),
                  [](llvm::StringRef Str) { return Str.trim().str(); });
}

void PhasarConfig::loadLLVMSpecialFunctionNames() {
  auto LLVMIntrinsics =
      readConfigFileAsTextOrErr(LLVMIntrinsicFunctionListFileName);
  if (!LLVMIntrinsics) {
    if (LLVMIntrinsics.getError() != std::errc::no_such_file_or_directory) {
      PHASAR_LOG_LEVEL(WARNING,
                       "Could not open config file '"
                           << LLVMIntrinsicFunctionListFileName
                           << "': " << LLVMIntrinsics.getError().message());
    }
    // Add default LLVM function names
    SpecialFuncNames.insert({"llvm.va_start"});
    return;
  }

  llvm::SmallVector<llvm::StringRef, 0> LLVMIntrinsicFunctions;
  llvm::SplitString(*LLVMIntrinsics, LLVMIntrinsicFunctions, "\n");

  llvm::transform(LLVMIntrinsicFunctions,
                  std::inserter(SpecialFuncNames, SpecialFuncNames.end()),
                  [](llvm::StringRef Str) { return Str.trim().str(); });
}

PhasarConfig &PhasarConfig::getPhasarConfig() {
  static PhasarConfig PC{};
  return PC;
}

} // namespace psr
