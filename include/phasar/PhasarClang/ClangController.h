/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARCLANG_CLANGCONTROLLER_H_
#define PHASAR_PHASARCLANG_CLANGCONTROLLER_H_

namespace clang {
namespace tooling {
class CommonOptionsParser;
}
} // namespace clang

namespace psr {

class ClangController {
public:
  ClangController(clang::tooling::CommonOptionsParser &OptionsParser);
};

} // namespace psr

#endif
