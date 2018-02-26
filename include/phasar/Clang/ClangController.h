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

#include "../utils/Logger.h"
#include "RandomChangeFrontendAction.h"
#include <iostream>

class ClangController {
public:
  ClangController(clang::tooling::CommonOptionsParser &OptionsParser);
};

#endif
