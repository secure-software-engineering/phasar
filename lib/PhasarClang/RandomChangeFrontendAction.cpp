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

#include <memory>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/CommandLine.h"

#include "phasar/PhasarClang/RandomChangeASTConsumer.h"
#include "phasar/PhasarClang/RandomChangeFrontendAction.h"

using namespace std;
using namespace psr;

namespace psr {

RandomChangeFrontendAction::RandomChangeFrontendAction() = default;

void RandomChangeFrontendAction::EndSourceFileAction() {
  clang::SourceManager &SM = RW.getSourceMgr();
  llvm::errs() << "** EndSourceFileAction for: "
               << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
  RW.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
}

std::unique_ptr<clang::ASTConsumer>
RandomChangeFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI,
                                              llvm::StringRef File) {
  llvm::errs() << "** Creating AST consumer for: " << File << "\n";
  RW.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  return std::make_unique<RandomChangeASTConsumer>(RW);
}
} // namespace psr
