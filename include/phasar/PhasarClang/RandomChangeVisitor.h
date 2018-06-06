/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyVisitor.hh
 *
 *  Created on: 10.06.2016
 *      Author: pdschbrt
 */

#ifndef CLANG_RANDOMCHANGEVISITOR_H_
#define CLANG_RANDOMCHANGEVISITOR_H_

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <random>
#include <sstream>
#include <string>

namespace psr {

// std::random_device rd;
// std::mt19937_64 mt(rd());
// std::uniform_int_distribution<int> dist(0, 2);
// for (int i = 0; i < 10; ++i) {
//   cout << dist(mt) << ' ';
// }
// cout << '\n';

class RandomChangeVisitor
    : public clang::RecursiveASTVisitor<RandomChangeVisitor> {
private:
  clang::Rewriter &RW;

public:
  RandomChangeVisitor(clang::Rewriter &R);
  virtual bool VisitVarDecl(clang::VarDecl *V);
  virtual bool VisitTypeDecl(clang::TypeDecl *T);
  virtual bool VisitStmt(clang::Stmt *S);
  virtual bool VisitFunctionDecl(clang::FunctionDecl *F);
};

} // namespace psr

#endif /* CLANG_MYVISITOR_HH_ */
