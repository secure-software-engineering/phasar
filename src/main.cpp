#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
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
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory("Static Analysis");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;
const static string MoreHelp(
    "\n"
    "======================================================\n"
    "=== Data-flow Analysis for C and C++ - User Manual ===\n"
    "======================================================\n\n"
    "There are currently two modes available to run a program analysis:\n\n"
    "1.) Single module analysis\n"
    "--------------------------\n"
    "Using the single module analysis the analysis tool expects at least the path "
    "to a C or C++ module. The module can be a plain C or C++ file (.c/.cpp), LLVM IR "
    "as a human readable .ll file or as bitcode format .bc.\n\n"
    "2.) Whole project analysis\n"
    "--------------------------\n"
    "This mode analyzes a whole C or C++ project consisting of multiple modules. "
    "It expects at least the path to a C or C++ project containing a 'compile_commands.json' "
    "file, which contains all compile commands of the project. This database can be generated automatically "
    "by using the cmake flag 'CMAKE_EXPORT_COMPILE_COMMANDS'. When make is used the bear tool can be used "
    "in order to generate the compile commands database. Our analysis tool reads the generated database "
    "to understand the project structure. It then compiles every C or C++ module that belongs to the project "
    " under analysis and compiles it to LLVM IR which is then stored in-memory for further preprocessing.\n\n"
    "Analysis Modes\n"
    "--------------\n"
    "Without specifying further parameters, our analysis tool tries to run all available analyses on the "
    "code that the user provides. If the user wishes otherwise, they must provide further parameters specifying "
    "the specific analysis to run. Currently the following analyses are available and can be choosen by using the "
    "parameters as shown in the following:\n\n"
    "\tanalysis - parameter\n"
    "\tuninitialized variable analysis (IFDS) - 'ifds_uninit'\n"
    "\ttaint analysis (IFDS) - 'ifds_taint'\n"
    "\ttaint analysis (IDE) - 'ide_taint'\n"
    "\ttype analysis (IFDS) - 'ifds_type'\n"
    "\n\n"
    "Of course the use can choose more than one analysis to be run on the code."
    "\n\n"
    "Gernal Workflow\n"
    "---------------\n"
    "TODO: decribe the general workflow!\n\n"
		"============================\n"
    "=== Command-line options ===\n"
		"============================\n");

// initialize the module passes ID's that we are using
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;

namespace boost {
	void throw_exception(std::exception const & e) {}
}


int main(int argc, const char **argv) {
	string ModulePath;
	string ProjectPath;
	bool WPAMode;
	vector<string> Analyses;

	try {
		bpo::options_description Description(MoreHelp+"\n\nCommand-line options");
		Description.add_options()
  				("help,h", "Print help message")
					("module,m", bpo::value<string>(&ModulePath), "Module for single module mode")
					("project,p", bpo::value<string>(&ProjectPath), "Path to the project under analysis")
					("wpa,w", bpo::value<bool>(&WPAMode)->default_value(1), "WPA mode (1 or 0)")
					("analysis,a", bpo::value<vector<string>>(&Analyses)->multitoken()->zero_tokens()->composing(), "Analysis");
		bpo::variables_map VarMap;
		bpo::store(bpo::parse_command_line(argc, argv, Description), VarMap);
		bpo::notify(VarMap);
		if (VarMap.count("help")) {
			cout << Description << "\n";
			return 0;
		}
	} catch (const bpo::error& e) {
		cerr << "error: could not parse command-line parameters\n"
						"message: " << e.what() << ", abort\n";
		return 1;
	}

  vector<AnalysisType> ChosenAnalyses = { AnalysisType::IFDS_UninitializedVariables,
                                           AnalysisType::IFDS_TaintAnalysis,
                                           AnalysisType::IDE_TaintAnalysis,
                                           AnalysisType::IFDS_TypeAnalysis };
  if (!Analyses.empty()) {
    ChosenAnalyses.clear();
  	for (auto& Analysis : Analyses) {
  		if (Analysis == "ifds_uninit")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_UninitializedVariables);
  		else if (Analysis == "ifds_taint")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_TaintAnalysis);
  		else if (Analysis == "ifds_type")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_TypeAnalysis);
  		else if (Analysis == "ide_taint")
  			ChosenAnalyses.push_back(AnalysisType::IDE_TaintAnalysis);
  		else if (Analysis == "ifds_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_SolverTest);
  		else if (Analysis == "ide_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::IDE_SolverTest);
  		else if (Analysis == "none")
  			ChosenAnalyses.push_back(AnalysisType::None);
  		else {
  			cerr << "error: unrecognized analysis type, abort\n";
  			return 1;
  		}
    }
  }

  if (ModulePath.empty() != ProjectPath.empty()) {
  	if (!ModulePath.empty()) {
  		if (!(bfs::exists(ModulePath) && !bfs::is_directory(ModulePath))) {
  			cerr << "error: '" << ModulePath << "' is not a valid file, abort\n";
  			return 1;
  		}
  		vector<const char*> CompileArgs;
  		ProjectIRCompiledDB IRDB(ModulePath, CompileArgs);
  		AnalysisController Controller(IRDB, ChosenAnalyses, WPAMode);
  	} else {
  		if (!(bfs::exists(ProjectPath) && bfs::is_directory(ProjectPath))) {
  			cerr << "error: '" << ProjectPath << "' is not a valid directory, abort\n";
  			return 1;
  		}
  		// perfrom a little trick to make OptionsParser only responsible for the project sources
  		int OnlyTakeCareOfSources = 2;
  		const char* ProjectSources = ProjectPath.c_str();
  		const char* DummyProgName = "not_important";
  		const char* DummyArgs[] = { DummyProgName, ProjectSources };
  		clang::tooling::CommonOptionsParser OptionsParser(OnlyTakeCareOfSources,
  																											DummyArgs,
																												StaticAnalysisCategory,
																												OccurrencesFlag);
  		clang::tooling::CompilationDatabase& CompileDB = OptionsParser.getCompilations();
  		ProjectIRCompiledDB IRDB(CompileDB);
  		AnalysisController Controller(IRDB, ChosenAnalyses, WPAMode);
  	}
  } else {
  	cerr << "error: expected at least the specification of parameter 'module' or 'project'\n"
  					"note: analysis can only be a single module OR whole project analysis\n"
  					"use --help to show help message, abort\n";
  	return 1;
  }
  llvm::llvm_shutdown();
  cout << "... shutdown analysis ..." << endl;
  return 0;
}
