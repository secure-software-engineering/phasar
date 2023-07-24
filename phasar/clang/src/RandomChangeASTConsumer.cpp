/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyASTConsumer.cpp
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#include "clang/AST/ASTContext.h"
// #include "clang/Frontend/CompilerInstance.h"

#include "phasar/PhasarClang/RandomChangeASTConsumer.h"
#include "phasar/PhasarClang/RandomChangeVisitor.h"

using namespace psr;

namespace psr {

RandomChangeASTConsumer::RandomChangeASTConsumer(clang::Rewriter &R)
    : Visitor(R) {}

void RandomChangeASTConsumer::HandleTranslationUnit(
    clang::ASTContext &Context) {
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}
} // namespace psr
