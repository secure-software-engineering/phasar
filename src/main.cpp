#include <boost/throw_exception.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include "analysis/AnalysisController.hh"
#include "analysis/passes/GeneralStatisticsPass.hh"
#include "analysis/passes/ValueAnnotationPass.hh"
#include "db/ProjectIRCompiledDB.hh"
#include "utils/utils.hh"

// this project is pretty large at this point, to avoid
// confusion only use the 'using namespace' command for the STL
// all other scopes shall be used explicitly
using namespace std;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory(
    "Data-flow Analysis for C and C++");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp(
    "+++ User Manual +++\n"
    "===================\n\n"
    "There are currently two modes available to run a program analysis:\n\n"
    "1.) Single module analysis\n"
    "--------------------------\n"
    "Using the single module analysis the analysis tool expects at least the path "
    "to a C or C++ module. The module can be a plain C or C++ file (.c/.cpp), LLVM IR "
    "as a human readable .ll file or as bitcode format .bc. "
    "A typically usage would be the following:\n\n\t"
    "usage: <prog> <path to a C/C++ module> -- <additional parameters>\n\n"
    "2.) Whole project analysis\n"
    "--------------------------\n"
    "This mode analyzes a whole C or C++ project consisting of multiple modules. "
    "It expects at least the path to a C or C++ project containing a 'compile_commands.json' "
    "file, which contains all compile commands of the project. This database can be generated automatically "
    "by using the cmake flag 'CMAKE_EXPORT_COMPILE_COMMANDS'. When make is used the bear tool can be used "
    "in order to generate the compile commands database. Our analysis tool reads the generated database "
    "to understand the project structure. It then compiles every C or C++ module that belongs to the project "
    " under analysis and compiles it to LLVM IR which is then stored in-memory for further preprocessing. "
    "A typically usage would be the following:\n\n\t"
    "usage: <prog> <path to a C/C++ project>\n\n"
    "Analysis Modes\n"
    "==============\n"
    "Without specifying further parameters, our analysis tool tries to run all available analyses on the "
    "code that the user provides. If the user wishes otherwise, they must provide further parameters specifying "
    "the specific analysis to run. Currently the following analyses are available and can be choosen by using the "
    "parameters as shown in the following:\n\n"
    "\tanalysis - parameter\n"
    "\tuninitialized variable analysis (IFDS) - '-ifds_uninit'\n"
    "\ttaint analysis (IFDS) - '-ifds_taint'\n"
    "\ttaint analysis (IDE) - '-ide_taint'\n"
    "\ttype analysis (IFDS) - '-ifds_type'\n"
    "\n\n"
    "Of course the use can choose more than one analysis to be run on the code."
    "\n\n"
    "Gernal Workflow\n"
    "===============\n"
    "TODO: decribe the general workflow!"
    "\n");
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;

// initialize the module passes ID's that we are using
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;

// this should be removed at some point!
void boost::throw_exception(std::exception const &e) {}


int main(int argc, const char **argv) {
  if (argc == 1) {
    cout << "error: too few arguments provided\n"
            "use '-help' for help\n"
            "abort!\n";
    return 1;
  }
  // set up the compile commands data base
  int only_take_care_of_sources = argc; // use that instread of argc, so we can handle the other parameters for ourself
  clang::tooling::CommonOptionsParser OptionsParser(only_take_care_of_sources,
                                                    argv,
                                                    StaticAnalysisCategory,
                                                    OccurrencesFlag);
  clang::tooling::CompilationDatabase &CompileDB = OptionsParser.getCompilations();

  // scan the other parameters to decide what analysis should be run
  vector<AnalysisKind> analyses_to_run = { AnalysisKind::IFDS_UninitializedVariables,
                                           AnalysisKind::IFDS_TaintAnalysis,
                                           AnalysisKind::IDE_TaintAnalysis,
                                           AnalysisKind::IFDS_TypeAnalysis };
  for (int i = 2; i < argc; ++i) {
      analyses_to_run.clear();
      string param(argv[i]);
      if (param == "-ifds_uninit") {
        analyses_to_run.push_back(AnalysisKind::IFDS_UninitializedVariables);
      } else if (param == "-ifds_taint") {
        analyses_to_run.push_back(AnalysisKind::IFDS_TaintAnalysis);
      } else if (param == "-ide_taint") {
        analyses_to_run.push_back(AnalysisKind::IDE_TaintAnalysis);
      } else if (param == "-ifds_type") {
        analyses_to_run.push_back(AnalysisKind::IFDS_TypeAnalysis);
      } else {
          cout << "error: unrecognized parameter '" << param << "'\n"
                  "use '-help' for help\n"
                  "abort!\n";
          return 1;
      }
  }
  cout << "starting the following analyses:\n";
  for (auto analysis : analyses_to_run) {
      cout << "\t" << analysis << "\n";
  }
  // create an 'in-memory' databse that is contains the raw front-end IR of all compilation modules
  cout << "compiling to LLVM IR ...\n";
  ProjectIRCompiledDB IRDB(CompileDB);

  cout << "compilation done, starting the analysis ...\n";
  AnalysisController Analysis(IRDB, analyses_to_run);

  // shutdown llvm
  llvm::llvm_shutdown();
  cout << "... shutdown analysis ..." << endl;
  return 0;
}
