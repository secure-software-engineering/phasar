/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IO.h
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_UTILS_IO_H
#define PHASAR_UTILS_IO_H

#include <filesystem>
#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "llvm/Support/MemoryBuffer.h"

namespace psr {

inline std::unique_ptr<llvm::MemoryBuffer> readFile(const llvm::Twine &Path) {
  auto Ret = llvm::MemoryBuffer::getFile(Path);

  if (!Ret) {
    throw std::system_error(Ret.getError());
  }

  return std::move(Ret.get());
}

inline std::string readTextFile(const llvm::Twine &Path) {
  auto Buffer = readFile(Path);
  return Buffer->getBuffer().str();
}
nlohmann::json readJsonFile(const llvm::Twine &Path);

void writeTextFile(const llvm::Twine &Path, llvm::StringRef Content);

} // namespace psr

#endif
