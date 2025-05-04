/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarClang/ClangController.h"

#include "phasar/PhasarClang/RandomChangeFrontendAction.h"
#include "phasar/Utils/Logger.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h" // clang::tooling::ClangTool, clang::tooling::newFrontendActionFactory

using namespace std;
using namespace psr;

namespace psr {

ClangController::ClangController(
    clang::tooling::CommonOptionsParser &OptionsParser) {
  PHASAR_LOG_LEVEL(DEBUG, "ClangController::ClangController()");
  PHASAR_LOG_LEVEL(DEBUG, "Source file(s):");
  for (auto Src : OptionsParser.getCompilations().getAllFiles()) {
    PHASAR_LOG_LEVEL(DEBUG, Src);
  }
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getCompilations().getAllFiles());
  Tool.run(
      clang::tooling::newFrontendActionFactory<RandomChangeFrontendAction>()
          .get());
  PHASAR_LOG_LEVEL(DEBUG, "finished clang ast analysis.");
}
} // namespace psr
