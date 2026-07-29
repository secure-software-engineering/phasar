/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/Timer.h"

#include "phasar.h"

#include <filesystem>
#include <string>

using namespace psr;

int main(int Argc, const char **Argv) {
  using namespace std::string_literals;

  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "myphasartool\n"
                    "A small PhASAR-based example program\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }

  std::vector EntryPoints = {"main"s};

  HelperAnalyses HA(Argv[1], EntryPoints);
  if (!HA.getProjectIRDB().isValid()) {
    return 1;
  }

  if (const auto *MainF = HA.getProjectIRDB().getFunctionDefinition("main")) {
    SimpleTimer Tm;

    std::ignore = computeAndersenOTFRaw(
        HA.getProjectIRDB(), {MainF}, nullptr, psr::Soundness::Soundy,
        ContextSensitivityOptions{
            .SelectionMode = psr::ContextSensitivityOptions::Mode::Dynamic,
        });

    llvm::errs() << "AndersenOTFAA elapsed: " << Tm.elapsed() << '\n';

  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
