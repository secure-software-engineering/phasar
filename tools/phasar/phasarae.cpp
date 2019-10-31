/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <phasar/Controller/AnalysisExecutor.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/Utils/Logger.h>
#include <boost/filesystem/operations.hpp>

namespace bfs = boost::filesystem;

using namespace std;
using namespace psr;

int main(int argc, const char **argv) {
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  DB.preprocessIR();
  if (DB.getFunction("main")) {
    AnalysisExecutor Exe;
    Exe.testExecutor(DB);
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
