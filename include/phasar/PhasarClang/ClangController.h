/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef CLANGCONTROLLER_H_
#define CLANGCONTROLLER_H_

#include <iostream>
#include <phasar/PhasarClang/RandomChangeFrontendAction.h>
#include <phasar/Utils/Logger.h>

class ClangController {
public:
  ClangController(clang::tooling::CommonOptionsParser &OptionsParser);
};

#endif
