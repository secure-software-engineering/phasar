/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "RandomChangeVisitor.h"

RandomChangeVisitor::RandomChangeVisitor(clang::Rewriter &R) : RW(R) {}

bool RandomChangeVisitor::VisitStmt(clang::Stmt *st) {
  // Only care about If statements.
  if (clang::isa<clang::IfStmt>(st)) {
    clang::IfStmt *IfStatement = clang::cast<clang::IfStmt>(st);
    clang::Stmt *Then = IfStatement->getThen();

    RW.InsertText(Then->getLocStart(), "// the 'if' part\n", true, true);

    clang::Stmt *Else = IfStatement->getElse();
    if (Else)
      RW.InsertText(Else->getLocStart(), "// the 'else' part\n", true, true);
  }

  return true;
}

bool RandomChangeVisitor::VisitFunctionDecl(clang::FunctionDecl *f) {
  // Only function definitions (with bodies), not declarations.
  if (f->hasBody()) {
    clang::Stmt *FuncBody = f->getBody();

    // Type name as string
    clang::QualType QT = f->getReturnType();
    std::string TypeStr = QT.getAsString();

    // Function name
    clang::DeclarationName DeclName = f->getNameInfo().getName();
    std::string FuncName = DeclName.getAsString();

    // Add comment before
    std::stringstream SSBefore;
    SSBefore << "// Begin function " << FuncName << " returning " << TypeStr
             << "\n";
    clang::SourceLocation ST = f->getSourceRange().getBegin();
    RW.InsertText(ST, SSBefore.str(), true, true);

    // And after
    std::stringstream SSAfter;
    SSAfter << "\n// End function " << FuncName;
    ST = FuncBody->getLocEnd().getLocWithOffset(1);
    RW.InsertText(ST, SSAfter.str(), true, true);
  }

  return true;
}

bool RandomChangeVisitor::VisitTypeDecl(clang::TypeDecl *t) { return false; }
