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

#include "RandomChangeASTConsumer.h"
#include "RandomChangeVisitor.h"

RandomChangeASTConsumer::RandomChangeASTConsumer(clang::Rewriter &R)
    : Visitor(R) {}

void RandomChangeASTConsumer::HandleTranslationUnit(
    clang::ASTContext &Context) {
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}
