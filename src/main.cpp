#include <algorithm>
#include <bitset>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Options.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/CFLAliasAnalysis.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include "analysis/AnalysisController.hh"
#include "analysis/call-points-to_graph/LLVMStructTypeHierarchy.hh"
#include "analysis/call-points-to_graph/VTable.hh"
#include "analysis/ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "analysis/ifds_ide/solver/LLVMIDESolver.hh"
#include "analysis/ifds_ide/solver/LLVMIFDSSolver.hh"
#include "analysis/ifds_ide_problems/ide_taint_analysis/IDETaintAnalysis.hh"
#include "analysis/ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.hh"
#include "analysis/ifds_ide_problems/ifds_uninitialized_variables/IFDSUninitializedVariables.hh"
#include "analysis/passes/GeneralStatisticsPass.hh"
#include "analysis/passes/ValueAnnotationPass.hh"
#include "clang/MyFrontendAction.hh"
#include "clang/MyMatcher.hh"
#include "clang/common.hh"
#include "db/DBConn.hh"
#include "db/ProjectIRCompiledDB.hh"
#include "utils/HexaStoreGraph.hh"
#include "utils/Singleton.hh"
#include "utils/utils.hh"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory(
    "Data-flow Analysis for C and C++");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp(
    "usage: <prog> <path to project containing a 'compile_commands.json' "
    "database file>\n"
    "use 'test_examples/simple_project' for example\n"
    "TODO: add a detailed describtion of what this analysis tool is doing and "
    "how it works!\n\n"
    "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy "
    "eirmod tempor "
    "invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At "
    "vero eos et accusam "
    "et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea "
    "takimata sanctus est "
    "Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur "
    "sadipscing elitr, sed "
    "diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam "
    "erat, sed diam "
    "voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet "
    "clita kasd "
    "gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet."
    "\n");
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;

// initialize the pass ID's to use
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;

namespace boost {
// this should be removed at some point!
void throw_exception(std::exception const &e) {}
}

int main(int argc, const char **argv) {
  if (argc == 1) {
    cout << "error: too few arguments provided\n"
            "use '-help' for help\n"
            "about!\n";
            return 1;
  }
  // set up the compile commands data base
  clang::tooling::CommonOptionsParser OptionsParser(
      argc, argv, StaticAnalysisCategory, OccurrencesFlag);
  clang::tooling::CompilationDatabase &CompileDB =
      OptionsParser.getCompilations();
  // create an 'in-memory' databse that is contains the raw front-end IR of all
  // compilation modules
  ProjectIRCompiledDB IRDB(CompileDB);
  IRDB.print();

  AnalysisController Analysis(
      IRDB, {AnalysisKind::IFDS_TaintAnalysis, AnalysisKind::IDE_TaintAnalysis,
             AnalysisKind::IFDS_UninitializedVariables});

   // Why the heck does the call to 'llvm_shutdown()' causes a 'corrupted
  // double-linked list'?
  // This should definitely be investigated at some point in time.
  //	llvm_shutdown();
  cout << "... shutdown analysis ..." << endl;
  return 0;
}
