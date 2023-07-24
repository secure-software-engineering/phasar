/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IO.cpp
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#include "phasar/Utils/IO.h"

#include "phasar/Utils/ErrorHandling.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

llvm::ErrorOr<std::string> psr::readTextFileOrErr(const llvm::Twine &Path) {
  auto BufferOrErr = readFileOrErr(Path);
  if (BufferOrErr) {
    return BufferOrErr->get()->getBuffer().str();
  }

  return BufferOrErr.getError();
}

llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
psr::readFileOrErr(const llvm::Twine &Path) noexcept {
  return llvm::MemoryBuffer::getFile(Path);
}

std::string psr::readTextFile(const llvm::Twine &Path) {
  return getOrThrow(readTextFileOrErr(Path));
}

std::unique_ptr<llvm::MemoryBuffer> psr::readFile(const llvm::Twine &Path) {
  return getOrThrow(readFileOrErr(Path));
}

std::optional<std::string> psr::readTextFileOrNull(const llvm::Twine &Path) {
  return getOrNull(readTextFileOrErr(Path));
}

std::unique_ptr<llvm::MemoryBuffer>
psr::readFileOrNull(const llvm::Twine &Path) noexcept {
  return getOrEmpty(readFileOrErr(Path));
}

nlohmann::json psr::readJsonFile(const llvm::Twine &Path) {
  auto Buf = readFile(Path);
  assert(Buf && "File reading failure should already be caught");
  return nlohmann::json::parse(Buf->getBufferStart(), Buf->getBufferEnd());
}

void psr::writeTextFile(const llvm::Twine &Path, llvm::StringRef Content) {
  std::error_code EC;
  llvm::SmallString<256> Buf;
  llvm::raw_fd_ostream ROS(Path.toNullTerminatedStringRef(Buf), EC);

  if (EC) {
    throw std::system_error(EC);
  }

  ROS.write(Content.data(), Content.size());
}

std::unique_ptr<llvm::raw_fd_ostream>
psr::openFileStream(const llvm::Twine &Filename) {
  std::error_code EC;
  llvm::SmallString<256> Buf;
  auto OFS = std::make_unique<llvm::raw_fd_ostream>(
      Filename.toNullTerminatedStringRef(Buf), EC);
  if (EC) {
    OFS = nullptr;
    PHASAR_LOG_LEVEL(INFO,
                     "Failed to open file: " << Buf << "; " << EC.message());
  }
  return OFS;
}
