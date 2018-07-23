/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyASTConsumer.hh
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#pragma once

#include <clang/AST/ASTConsumer.h>

#include "RandomChangeVisitor.h"

namespace clang {
  class ASTContext;
  class Rewriter;
}

namespace psr {

class RandomChangeASTConsumer : public clang::ASTConsumer {
private:
  RandomChangeVisitor Visitor;

public:
  RandomChangeASTConsumer(clang::Rewriter &R);

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

  // virtual bool HandleTopLevelDecl(DeclGroupRef DG);
};

} // namespace psr
