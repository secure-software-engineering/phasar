/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyMatcher.hh
 *
 *  Created on: 15.06.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARCLANG_MYMATCHER_H_
#define PHASAR_PHASARCLANG_MYMATCHER_H_

#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace psr {

class MyMatcher : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);
};

} // namespace psr

#endif
