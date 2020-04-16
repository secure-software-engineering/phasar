/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyMatcher.cpp
 *
 *  Created on: 15.06.2016
 *      Author: pdschbrt
 */

#include <iostream>
#include <string>

#include "clang/ASTMatchers/ASTMatchers.h"

#include "phasar/PhasarClang/MyMatcher.h"
using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace psr;

namespace psr {

// StatementMatcher myStmtMatcher =
// forStmt(hasLoopInit(declStmt(hasSingleDecl(varDecl(hasInitializer(integerLiteral(equals(0)))))))).bind("forLoop");
////StatementMatcher myCallMatcher = callExpr(isExpansionInMainFile(),
/// callee(cxxMethodDecl(hasName("foo"))),
/// hasAncestor(recordDecl().bind("caller"))).bind("callee");
// StatementMatcher myCallMatcher =
// callExpr(hasAncestor(functionDecl().bind("caller")),
// callee(functionDecl().bind("callee"))).bind("call");
//
void MyMatcher::run(const MatchFinder::MatchResult &Result) {
  if (const auto *FS = Result.Nodes.getNodeAs<ForStmt>("forLoop")) {
    FS->dump();
  }
  if (const auto *CALLEE = Result.Nodes.getNodeAs<CallExpr>("callee")) {
    errs() << "CALLEE: "
           << CALLEE->getDirectCallee()->getNameInfo().getAsString() << "\n";
  }
  if (const auto *CALLER = Result.Nodes.getNodeAs<CXXRecordDecl>("caller")) {
    errs() << "CALLER: " << CALLER->getNameAsString() << "\n";
  }
  if (const auto *Caller =
          Result.Nodes.getNodeAs<clang::FunctionDecl>("caller")) {
    errs() << "### Caller:" << Caller->getNameInfo().getAsString() << "\n";
  }
  if (const auto *Callee =
          Result.Nodes.getNodeAs<clang::FunctionDecl>("callee")) {
    errs() << "### Callee:" << Callee->getNameInfo().getAsString() << "\n";
  }
  if (const auto *Call = Result.Nodes.getNodeAs<clang::CallExpr>("call")) {
    errs() << "### with num args: " << Call->getNumArgs() << "\n";
  }
}
//
////void dumpdiag(clang::SourceRange R) {
////    clang::TextDiagnostic TD(llvm::outs(), AST->getLangOpts(),
////                             &AST->getDiagnostics().getDiagnosticOptions());
////    TD.emitDiagnostic(R.getBegin(), DiagnosticsEngine::Note,
////                      "Occurance found here",
/// CharSourceRange::getTokenRange(R),
////                      None, &AST->getSourceManager());
////  }
} // namespace psr
