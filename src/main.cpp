#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/throw_exception.hpp>

#include <clang/Driver/Driver.h>
#include <clang/Driver/Options.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/CFLAliasAnalysis.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/PassSupport.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "utils/utils.hh"
#include "utils/HexaStoreGraph.hh"
#include "utils/Singleton.hh"
#include "clang/MyFrontendAction.hh"
#include "clang/MyMatcher.hh"
#include "clang/common.hh"
#include "db/DBConn.hh"
#include "analysis/call-points-to_graph/LLVMStructTypeHierarchy.hh"
#include "analysis/call-points-to_graph/VTable.hh"
#include "analysis/ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "analysis/passes/ValueAnnotationPass.hh"
#include "analysis/passes/GeneralStatisticsPass.hh"
#include "analysis/ifds_ide/solver/LLVMIFDSSolver.hh"
#include "analysis/ifds_ide/solver/LLVMIDESolver.hh"
#include "analysis/ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.hh"
#include "analysis/ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.hh"
#include "analysis/ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.hh"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory("static analysis tool");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("More help text...\n");
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::OneOrMore;


// initialize MyHelloPass ID
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;


inline bool isInterestingPointer(Value* V)
{
	return V->getType()->isPointerTy() && !isa<ConstantPointerNull>(V);
}

namespace boost
{
	// this should be removed at some point!
	void throw_exception(std::exception const &e){}
}

int main(int argc, const char **argv)
{
////	// set up the compile commands data base
//	CommonOptionsParser OptionsParser(argc, argv, StaticAnalysisCategory, OccurrencesFlag);
//	CompilationDatabase& CompileDB = OptionsParser.getCompilations();
//
//	for (auto file : CompileDB.getAllFiles()) {
//		cout << file << endl;
//		vector<string> args;
//		cout << "with compile command: " << endl;
//		auto compilecommands = CompileDB.getCompileCommands(file);
//		vector<const char*> cargs;
//		for (auto compilecommand : compilecommands) {
//			for (auto stuff : compilecommand.CommandLine) {
//				cargs.push_back(stuff.c_str());
//			}
//		}
//		clang::CompilerInvocation* CI = createInvocationFromCommandLine(cargs);
//		clang::CompilerInstance Clang;
//		Clang.setInvocation(CI);
//		LLVMContext Context;
//		unique_ptr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction(&Context));
//		if (!Clang.ExecuteAction(*Act)) {
//			cout << "execute action went wrong!" << endl;
//			return 1;
//		}
//		unique_ptr<llvm::Module> module = Act->takeModule();
//		if (module != nullptr) {
//			module->dump();
//		} else {
//			cout << "could not compile module" << endl;
//		}
//
//		cout << "-------------------" << endl;
//	}


//	//
//	// set up the diagnostic engine in order to report problems
//	// Path to the C file
//	string inputPath = "getinmemory.c";
//	// Arguments to pass to the clang frontend
//	vector<const char *> args;
////	args.push_back(inputPath.c_str());
//	// The compiler invocation needs a DiagnosticsEngine so it can report problems
//	clang::TextDiagnosticPrinter *DiagClient = new clang::TextDiagnosticPrinter(llvm::errs(), new clang::DiagnosticOptions());
//	llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
//	clang::DiagnosticsEngine Diags(DiagID, new clang::DiagnosticOptions());
//	// Create the compiler invocation
//	//unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation());
//	vector<const char*> arg = { "/home/pdschbrt/GIT-Repos/sse_dfa_cxx/sse_utils/bin/getinmemory.c" };
//	//clang::CompilerInvocation* CI = createInvocationFromCommandLine(arg); //::CreateFromArgs(*CI, args.begin(), args.end(), Diags);
//	clang::CompilerInstance Clang;
//	Clang.createDiagnostics();
//
//	clang::CompilerInvocation* CI = new clang::CompilerInvocation;
//	CompilerInvocation::CreateFromArgs(*CI, &arg[0], &arg[0]+1, Diags);
//	Clang.setInvocation(CI);
//
//	unique_ptr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());
//	if (!Clang.ExecuteAction(*Act)) {
//		cout << "could not compile module!!!" << endl;
//		return 1;
//	}
//	unique_ptr<llvm::Module> module = Act->takeModule();
//	if (module != nullptr) {
//		module->dump();
//	} else {
//		cout << "could not compile module" << endl;
//	}


	if (argc < 2) {
		cout << "usage: <prog> <a LLVM IR file>" << endl;
		return 1;
	}
	SMDiagnostic ErrorDiagnostic;
	LLVMContext Context;
	unique_ptr<llvm::Module> Mod(parseIRFile(argv[1], ErrorDiagnostic, Context));
	if (!Mod) {
		ErrorDiagnostic.print(argv[0], errs());
		return 1;
	}

//	PassManagerBuilder Builder;
//	Builder.populateModulePassManager();
	legacy::PassManager PM;
	CFLAAWrapperPass* CFLAAPass = new CFLAAWrapperPass();
	AAResultsWrapperPass* AARWP = new AAResultsWrapperPass();
//	Just commented out for testing purposes
//	PM.add(createPromoteMemoryToRegisterPass());
	PM.add(CFLAAPass);
	PM.add(AARWP);
	PM.add(new ValueAnnotationPass(Context));
	PM.add(new GeneralStatisticsPass());
	PM.run(*Mod);
	// just to be sure
	bool isInValid = verifyModule(*Mod);
	if (isInValid) {
		outs() << "ERROR: module is not valid!\n";
		return 1;
	}

	Mod->dump();

	/*
	 * TODO: change 'createZeroValue()'
	 * A user can only create one analysis problem at a time, due to the implementation of 'createZeroValue()'.
	 * This has to be changed of course!
//	 */

	LLVMBasedInterproceduralICFG icfg(*Mod);
	IFDSUnitializedVariables uninitializedvarproblem(icfg, Context);
	LLVMIFDSSolver<const llvm::Value*, LLVMBasedInterproceduralICFG&> llvmunivsolver(uninitializedvarproblem, true);
	llvmunivsolver.solve();
	cout << "back in main, solving done!" << endl;

	//llvm_shutdown();
	return 0;
}
