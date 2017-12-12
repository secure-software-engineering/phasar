/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyVisitor.hh
 *
 *  Created on: 10.06.2016
 *      Author: pdschbrt
 */

#ifndef CLANG_MYVISITOR_HH_
#define CLANG_MYVISITOR_HH_

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <string>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

// class MyVisitor : public RecursiveASTVisitor<MyVisitor> {
// private:
//    ASTContext *astContext; // used for getting additional AST info
//
// public:
//    explicit MyVisitor(CompilerInstance *CI);
//    virtual bool VisitFunctionDecl(FunctionDecl *func);
////    virtual bool VisitStmt(Stmt *st);
//
///*
//    virtual bool VisitReturnStmt(ReturnStmt *ret) {
//        rewriter.ReplaceText(ret->getRetValue()->getLocStart(), 6, "val");
//        errs() << "** Rewrote ReturnStmt\n";
//        return true;
//    }
//*/
////    virtual bool VisitCallExpr(CallExpr *call);
////    virtual bool VisitCXXRecordDecl(CXXRecordDecl *Declaration);
//    virtual bool VisitRecordDecl(RecordDecl *Declaration);
//
//};

#endif /* CLANG_MYVISITOR_HH_ */
