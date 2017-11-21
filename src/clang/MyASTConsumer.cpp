/*
 * MyASTConsumer.cpp
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#include "MyASTConsumer.hh"
#include "MyVisitor.hh"

//// override the constructor in order to pass CI
// MyASTConsumer::MyASTConsumer(CompilerInstance *CI) : visitor(new
// MyVisitor(CI)) // initialize the visitor
//{
//
//}
//
// void MyASTConsumer::HandleTranslationUnit(ASTContext &Context)
//{
//        /* we can use ASTContext to get the TranslationUnitDecl, which is
//             a single Decl that collectively represents the entire source file
//             */
//	visitor->TraverseDecl(Context.getTranslationUnitDecl());
//}
