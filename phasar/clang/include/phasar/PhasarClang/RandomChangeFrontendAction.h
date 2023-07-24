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

#ifndef PHASAR_PHASARCLANG_RANDOMCHANGEFRONTENDACTION_H_
#define PHASAR_PHASARCLANG_RANDOMCHANGEFRONTENDACTION_H_

#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/StringRef.h"

#include <memory>

namespace clang {
class CompilerInstance;
} // namespace clang

namespace psr {

class RandomChangeFrontendAction : public clang::ASTFrontendAction {
private:
  clang::Rewriter RW;

public:
  RandomChangeFrontendAction();

  void EndSourceFileAction() override;

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef File) override;
};

} // namespace psr

#endif
