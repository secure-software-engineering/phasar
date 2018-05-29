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

#ifndef CLANG_MYASTCONSUMER_H_
#define CLANG_MYASTCONSUMER_H_

#include "RandomChangeVisitor.h"
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
namespace psr{

class RandomChangeASTConsumer : public clang::ASTConsumer {
private:
  RandomChangeVisitor Visitor;

public:
  RandomChangeASTConsumer(clang::Rewriter &R);

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

  // virtual bool HandleTopLevelDecl(DeclGroupRef DG);
};

}//namespace psr

#endif /* CLANG_MYASTCONSUMER_HH_ */
