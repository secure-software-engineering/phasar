/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * RandomChangeFrontendAction.cpp
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#include "RandomChangeFrontendAction.h"

RandomChangeFrontendAction::RandomChangeFrontendAction() {}

void RandomChangeFrontendAction::EndSourceFileAction() {
  clang::SourceManager &SM = RW.getSourceMgr();
  llvm::errs() << "** EndSourceFileAction for: "
               << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

  // Now emit the rewritten buffer.
  RW.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
}

std::unique_ptr<clang::ASTConsumer>
RandomChangeFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI,
                                              llvm::StringRef file) {
  llvm::errs() << "** Creating AST consumer for: " << file << "\n";
  RW.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  return llvm::make_unique<RandomChangeASTConsumer>(RW);
}
