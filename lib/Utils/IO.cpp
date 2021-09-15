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

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/Utilities.h"

namespace psr {

std::string readTextFile(const std::filesystem::path &Path) {
  if (!(std::filesystem::exists(Path) &&
        std::filesystem::is_regular_file(Path))) {
    throw std::ios_base::failure("File does not exist: " + Path.string());
  }

  auto NumBytes = std::filesystem::file_size(Path);

  auto *F = std::fopen(Path.c_str(), "r");

  if (!F) {
    throw std::ios_base::failure("Could not open file: " + Path.string());
  }

  auto CloseFile = scope_exit([F]() { std::fclose(F); });

  std::string Contents;

  /// TODO: Get rid of the zero-initialization of the string
  Contents.resize(NumBytes);

  auto ReadBytes = std::fread(Contents.data(), 1, NumBytes, F);

  if (ReadBytes != NumBytes) {
    throw std::ios_base::failure("Could not read file: " + Path.string());
  }

  return Contents;
}

void writeTextFile(const std::filesystem::path &Path,
                   const std::string &Content) {
  std::error_code EC;
  llvm::raw_fd_ostream ROS(Path.string(), EC);

  if (EC) {
    throw std::ios_base::failure("Error creating the file: " + Path.string() +
                                 "; " + EC.message());
  }

  ROS.write(Content.data(), Content.size());
}

} // namespace psr
