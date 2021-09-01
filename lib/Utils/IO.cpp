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

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

#include "phasar/Utils/IO.h"

namespace psr {

std::string readTextFile(const std::string &Path) {
  if (!(std::filesystem::exists(Path) &&
        std::filesystem::is_regular_file(Path))) {
    throw std::ios_base::failure("file does not exist: " + Path);
  }
  std::ifstream Ifs(Path);
  if (!Ifs) {
    throw std::ios_base::failure("could not open file: " + Path);
  }
  std::stringstream StrStream;
  std::string Contents;
  StrStream << Ifs.rdbuf();
  Contents = StrStream.str();
  return Contents;
}

void writeTextFile(const std::string &Path, const std::string &Content) {
  std::ofstream Ofs(Path, std::ios::binary);
  if (Ofs.is_open()) {
    Ofs.write(Content.data(), Content.size());
  }
  throw std::ios_base::failure("could not write file: " + Path);
}

} // namespace psr
