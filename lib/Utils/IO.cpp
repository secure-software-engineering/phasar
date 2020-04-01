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
    ifstream ifs(Path, ios::binary);
    if (ifs.is_open()) {
      ifs.seekg(0, ifs.end);
      size_t file_size = ifs.tellg();
      ifs.seekg(0, ifs.beg);
      string content;
      content.resize(file_size);
      ifs.read(const_cast<char *>(content.data()), file_size);
      return content;
    }
  }
  throw ios_base::failure("could not read file: " + Path);
}

void writeFile(const string &Path, const string &Content) {
  ofstream ofs(Path, ios::binary);
  if (ofs.is_open()) {
    ofs.write(Content.data(), Content.size());
  }
  throw ios_base::failure("could not write file: " + Path);
}
} // namespace psr
