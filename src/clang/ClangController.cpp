/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "ClangController.h"

ClangController::ClangController(
    clang::tooling::CommonOptionsParser &OptionsParser) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "ClangController::ClangController()";
  BOOST_LOG_SEV(lg, DEBUG) << "Source file(s):";
  // for (auto &src : OptionsParser.getSourcePathList()) {
  for (auto src : OptionsParser.getCompilations().getAllFiles()) {
    BOOST_LOG_SEV(lg, DEBUG) << src;
  }
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getCompilations().getAllFiles());
  int result = Tool.run(
      clang::tooling::newFrontendActionFactory<RandomChangeFrontendAction>()
          .get());
  BOOST_LOG_SEV(lg, DEBUG) << "finished clang ast analysis.";
}
