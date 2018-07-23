/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

namespace clang {
  namespace tooling {
    class CommonOptionsParser;
  }
}

namespace psr {

class ClangController {
public:
  ClangController(clang::tooling::CommonOptionsParser &OptionsParser);
};

} // namespace psr
