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

#include "llvm/Support/MemoryBuffer.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace psr {

std::string readTextFile(const llvm::Twine &Path);

std::unique_ptr<llvm::MemoryBuffer> readFile(const llvm::Twine &Path);

nlohmann::json readJsonFile(const llvm::Twine &Path);

void writeTextFile(const llvm::Twine &Path, llvm::StringRef Content);

} // namespace psr

#endif
