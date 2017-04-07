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
    "\n"
    "===================\n"
    "=== User Manual ===\n"
    "===================\n\n"
    "There are currently two modes available to run a program analysis:\n\n"
    "1.) Single module analysis\n"
    "--------------------------\n"
    "Using the single module analysis the analysis tool expects at least the path "
    "to a C or C++ module. The module can be a plain C or C++ file (.c/.cpp), LLVM IR "
    "as a human readable .ll file or as bitcode format .bc. "
    "A typically usage would be the following:\n\n\t"
    "usage: <prog> -module <path to a .c/.cpp/.ll module> <0 or more analyses> -- <additional compiler arguments>\n\n"
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
    "usage: <prog> <path to a C/C++ project> <0 or more analyses>\n\n"
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
namespace boost
{
void throw_exception(std::exception const &e) {}
}

int main(int argc, const char **argv) {
  if (argc == 1) {
    cout << "error: too few arguments provided\n"
            "use '-help' for help\n"
            "abort!\n";
    return 1;
  }
  // provide some default analyses
  vector<AnalysisType> default_analyses = { AnalysisType::IFDS_UninitializedVariables,
                                            AnalysisType::IFDS_TaintAnalysis,
                                            AnalysisType::IDE_TaintAnalysis,
                                            AnalysisType::IFDS_TypeAnalysis };
  // analyses choosen by the user
  vector<AnalysisType> user_analyses;
  // single module mode
  if (argc >= 2 && string(argv[1]) == "-module") {
    string path(argv[2]);
    int additional_params_idx = argc;
    vector<const char*> compile_args;
    for (int i = 3; i < argc; ++i) {
      string param(argv[i]);
      if (param == "-ifds_uninit") {
        user_analyses.push_back(AnalysisType::IFDS_UninitializedVariables);
      } else if (param == "-ifds_taint") {
        user_analyses.push_back(AnalysisType::IFDS_TaintAnalysis);
      } else if (param == "-ide_taint") {
        user_analyses.push_back(AnalysisType::IDE_TaintAnalysis);
      } else if (param == "-ifds_type") {
        user_analyses.push_back(AnalysisType::IFDS_TypeAnalysis);
      } else if (param == "--") {
        additional_params_idx = i;
        break;
      }
    }
    for (int i = additional_params_idx + 1; i < argc; ++i) {
      compile_args.push_back(argv[i]);
    }
    cout << "compiling to LLVM IR ...\n";
    ProjectIRCompiledDB IRDB(path, compile_args);
    cout << "starting the following analyses:\n";
    auto analyses = (user_analyses.empty()) ? default_analyses : user_analyses;
    for (auto analysis : analyses) {
      cout << "\t" << analysis << "\n";
    }
    cout << "compilation done, starting the analysis ...\n";
    AnalysisController Analysis(IRDB, analyses);
  } else {
    // use the project database
    int only_take_care_of_sources = 2; // use that instread of argc, so we can handle the other parameters for ourself
    clang::tooling::CommonOptionsParser OptionsParser(only_take_care_of_sources,
                                                      argv,
                                                      StaticAnalysisCategory,
                                                      OccurrencesFlag);
    clang::tooling::CompilationDatabase &CompileDB = OptionsParser.getCompilations();

    for (int i = 2; i < argc; ++i) {
        string param(argv[i]);
        if (param == "-ifds_uninit") {
          user_analyses.push_back(AnalysisType::IFDS_UninitializedVariables);
        } else if (param == "-ifds_taint") {
          user_analyses.push_back(AnalysisType::IFDS_TaintAnalysis);
        } else if (param == "-ide_taint") {
          user_analyses.push_back(AnalysisType::IDE_TaintAnalysis);
        } else if (param == "-ifds_type") {
          user_analyses.push_back(AnalysisType::IFDS_TypeAnalysis);
        } else {
            cout << "error: unrecognized parameter '" << param << "'\n"
                    "use '-help' for help\n"
                    "abort!\n";
            return 1;
        }
    }
    // create an 'in-memory' databse that is contains the raw front-end IR of all compilation modules
    cout << "compiling to LLVM IR ...\n";
    ProjectIRCompiledDB IRDB(CompileDB);
    cout << "starting the following analyses:\n";
    auto analyses = (user_analyses.empty()) ? default_analyses : user_analyses;
    for (auto analysis : analyses) {
      cout << "\t" << analysis << "\n";
    }
    cout << "compilation done, starting the analysis ...\n";
    AnalysisController Analysis(IRDB, analyses);
  }
  // shutdown llvm
  llvm::llvm_shutdown();
  cout << "... shutdown analysis ..." << endl;
  return 0;
}
