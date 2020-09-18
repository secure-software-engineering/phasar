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

#include <fstream>

#include "boost/filesystem.hpp"

#include "phasar/Utils/IO.h"
using namespace psr;
using namespace std;

namespace psr {

string readFile(const string &Path) {
  if (boost::filesystem::exists(Path) &&
      !boost::filesystem::is_directory(Path)) {
    ifstream Ifs(Path, ios::binary);
    if (Ifs.is_open()) {
      Ifs.seekg(0, std::ifstream::end);
      size_t FileSize = Ifs.tellg();
      Ifs.seekg(0, std::ifstream::beg);
      string Content;
      Content.resize(FileSize);
      Ifs.read(const_cast<char *>(Content.data()), FileSize);
      return Content;
    }
  }
  throw ios_base::failure("could not read file: " + Path);
}

void writeFile(const string &Path, const string &Content) {
  ofstream Ofs(Path, ios::binary);
  if (Ofs.is_open()) {
    Ofs.write(Content.data(), Content.size());
  }
  throw ios_base::failure("could not write file: " + Path);
}
} // namespace psr
