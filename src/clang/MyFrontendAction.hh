/*
 * MyFrontendAction.hh
 *
 *  Created on: 09.06.2016
 *      Author: pdschbrt
 */

#ifndef CLANG_MYFRONTENDACTION_HH_
#define CLANG_MYFRONTENDACTION_HH_

#include "MyASTConsumer.hh"
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <memory>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

class MyFrontendAction : public ASTFrontendAction {
public:
  virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                    StringRef file);
};

#endif /* CLANG_MYFRONTENDACTION_HH_ */
