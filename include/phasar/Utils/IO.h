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

std::string readTextFile(const std::filesystem::path &Path);

std::unique_ptr<llvm::MemoryBuffer> readFile(const std::filesystem::path &Path);
std::unique_ptr<llvm::MemoryBuffer> readFile(const llvm::Twine &Path);

nlohmann::json readJsonFile(const llvm::Twine &Path);
nlohmann::json readJsonFile(const std::filesystem::path &Path);

void writeTextFile(const std::filesystem::path &Path, llvm::StringRef Content);

} // namespace psr

#endif
