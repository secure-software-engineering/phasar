/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyFrontendAction.hh
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#ifndef CLANG_MYFRONTENDACTION_H_
#define CLANG_MYFRONTENDACTION_H_

#include "RandomChangeASTConsumer.h"
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <memory>

namespace psr {

class RandomChangeFrontendAction : public clang::ASTFrontendAction {
private:
  clang::Rewriter RW;

public:
  RandomChangeFrontendAction();

  void EndSourceFileAction() override;

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file) override;
};

} // namespace psr

#endif /* CLANG_MYFRONTENDACTION_HH_ */
