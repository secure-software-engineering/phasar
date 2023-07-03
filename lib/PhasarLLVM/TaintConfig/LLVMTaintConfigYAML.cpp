/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfigYAML.h"

#include "llvm/Support/ErrorHandling.h"

#include <fstream>

namespace psr {

void LLVMTaintConfigYAML::loadYAML(const llvm::Twine &Path) {
  std::ifstream File;
  File.open(Path.str().c_str());

  if (File.fail()) {
    llvm::report_fatal_error("File could not be opened: " + Path);
    return;
  }

  // if file exists and is openable, loop over file and extract all neccesary
  // information
  std::string Line;
  unsigned int LineCounter = 0;
  while (std::getline(File, Line)) {
    LineCounter++;
  }

  File.close();
}

} // namespace psr
