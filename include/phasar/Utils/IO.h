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

#include "llvm/ADT/StringRef.h"

namespace llvm {
class MemoryBuffer;
class raw_fd_ostream;
class Twine;
} // namespace llvm

namespace psr {

std::string readTextFile(const llvm::Twine &Path);

std::unique_ptr<llvm::MemoryBuffer> readFile(const llvm::Twine &Path);

nlohmann::json readJsonFile(const llvm::Twine &Path);

void writeTextFile(const llvm::Twine &Path, llvm::StringRef Content);

std::unique_ptr<llvm::raw_fd_ostream>
openFileStream(const llvm::Twine &Filename);

} // namespace psr

#endif
