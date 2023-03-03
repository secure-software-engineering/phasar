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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"

#include "nlohmann/json.hpp"

#include <optional>

namespace psr {

[[nodiscard]] llvm::ErrorOr<std::string>
readTextFileOrErr(const llvm::Twine &Path);
[[nodiscard]] llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
readFileOrErr(const llvm::Twine &Path) noexcept;

[[nodiscard]] std::string readTextFile(const llvm::Twine &Path);
[[nodiscard]] std::unique_ptr<llvm::MemoryBuffer>
readFile(const llvm::Twine &Path);

[[nodiscard]] std::optional<std::string>
readTextFileOrNull(const llvm::Twine &Path);
[[nodiscard]] std::unique_ptr<llvm::MemoryBuffer>
readFileOrNull(const llvm::Twine &Path) noexcept;

[[nodiscard]] nlohmann::json readJsonFile(const llvm::Twine &Path);

void writeTextFile(const llvm::Twine &Path, llvm::StringRef Content);

[[nodiscard]] std::unique_ptr<llvm::raw_fd_ostream>
openFileStream(const llvm::Twine &Filename);

} // namespace psr

#endif
