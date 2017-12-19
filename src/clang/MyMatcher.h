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

#ifndef CLANG_MYMATCHER_H_
#define CLANG_MYMATCHER_H_

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <iostream>
#include <string>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;

class MyMatcher : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result);
};

#endif /* CLANG_MYMATCHER_HH_ */
