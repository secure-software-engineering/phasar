/*
 * MyMatcher.hh
 *
 *  Created on: 15.06.2016
 *      Author: pdschbrt
 */

#ifndef CLANG_MYMATCHER_HH_
#define CLANG_MYMATCHER_HH_

#include <iostream>
#include <string>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;


class MyMatcher : public MatchFinder::MatchCallback {
public:
	virtual void run(const MatchFinder::MatchResult &Result);
};

#endif /* CLANG_MYMATCHER_HH_ */
