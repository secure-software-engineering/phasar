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

#ifndef CLANG_MYASTCONSUMER_HH_
#define CLANG_MYASTCONSUMER_HH_

#include "MyVisitor.hh"
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

using namespace clang;
using namespace llvm;

// class MyASTConsumer : public ASTConsumer {
// private:
//    MyVisitor *visitor; // doesn't have to be private
//
// public:
//    // override the constructor in order to pass CI
//    explicit MyASTConsumer(CompilerInstance *CI);
//
//    // override this to call our ExampleVisitor on the entire source file
//    virtual void HandleTranslationUnit(ASTContext &Context);
//
///*
//    // override this to call our ExampleVisitor on each top-level Decl
//    virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
//        // a DeclGroupRef may have multiple Decls, so we iterate through each
//        one
//        for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; i++)
//        {
//            Decl *D = *i;
//            visitor->TraverseDecl(D); // recursively visit each AST node in
//            Decl "D"
//        }
//        return true;
//    }
//*/
//};

#endif /* CLANG_MYASTCONSUMER_HH_ */
