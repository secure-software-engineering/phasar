/*
 * MyVisitor.cpp
 *
 *  Created on: 10.06.2016
 *      Author: pdschbrt
 */

#include "MyVisitor.hh"
#include "common.hh"

// MyVisitor::MyVisitor(CompilerInstance *CI) :
// astContext(&(CI->getASTContext())) // initialize private members
//{
//  rewriter.setSourceMgr(astContext->getSourceManager(),
//  astContext->getLangOpts());
//}
//
// bool MyVisitor::VisitFunctionDecl(FunctionDecl *func)
//{
//	++numFunctions;
//    string funcName = func->getNameInfo().getName().getAsString();
//    outs() << "@ Found function declaration: " << funcName << "\n";
//    return true;
//}
//
////bool MyVisitor::VisitCallExpr(CallExpr *call)
////{
////	FunctionDecl *func = call->getDirectCallee();
////	errs() << "** Found function call of: " <<
///func->getNameInfo().getName().getAsString() << "\n";
////	return true;
////}
//
////bool MyVisitor::VisitCXXRecordDecl(CXXRecordDecl *Declaration)
////{
////		FullSourceLoc fullLocation =
///astContext->getFullLoc(Declaration->getLocStart());
////        if (fullLocation.isValid())
////        	errs() << "Found declaration of record '"<<
///Declaration->getQualifiedNameAsString() << "' at "
////            	<< fullLocation.getSpellingLineNumber() << ":"
////                << fullLocation.getSpellingColumnNumber() << "\n";
////       return true;
////}
//
// bool MyVisitor::VisitRecordDecl(RecordDecl *Declaration)
//{
//	++numRecords;
//	string declName = Declaration->getDeclName().getAsString();
//	outs() << "# Found record declaration: " << declName << "\n";
//	return true;
//}

// bool MyVisitor::VisitFunctionDecl(FunctionDecl *func)
//{
//	numFunctions++;
//    string funcName = func->getNameInfo().getName().getAsString();
//    if (funcName == "do_math") {
//    	rewriter.ReplaceText(func->getLocation(), funcName.length(), "add5");
//        errs() << "** Rewrote function def: " << funcName << "\n";
//    }
//    return true;
//}

// bool MyVisitor::VisitStmt(Stmt *st) {
//    if (ReturnStmt *ret = dyn_cast<ReturnStmt>(st)) {
//        rewriter.ReplaceText(ret->getRetValue()->getLocStart(), 6, "val");
//        errs() << "** Rewrote ReturnStmt\n";
//    }
//    if (CallExpr *call = dyn_cast<CallExpr>(st)) {
//        rewriter.ReplaceText(call->getLocStart(), 7, "add5");
//        errs() << "** Rewrote function call\n";
//    }
//    return true;
//}
