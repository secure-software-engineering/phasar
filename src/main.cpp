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
static const string MoreHelp =
	#include "more_help.txt"
	;

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
	bool Mem2Reg;
	bool PrintEdgeRecorder;

	try {
		bpo::options_description Description(MoreHelp+"\n\nCommand-line options");
		Description.add_options()
  				("help,h", "Print help message")
					("module,m", bpo::value<string>(&ModulePath), "Module for single module mode")
					("project,p", bpo::value<string>(&ProjectPath), "Path to the project under analysis")
					("analysis,a", bpo::value<vector<string>>(&Analyses)->multitoken()->zero_tokens()->composing(), "Analysis")
					("wpa,w", bpo::value<bool>(&WPAMode)->default_value(1), "WPA mode (1 or 0)")
					("mem2reg", bpo::value<bool>(&Mem2Reg)->default_value(1), "Promote memory to register pass (1 or 0)")
					("printedgerec,r", bpo::value<bool>(&PrintEdgeRecorder)->default_value(0), "Print edge recorder (1 or 0)");
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

  vector<AnalysisType> ChosenAnalyses = { AnalysisType::IFDS_SolverTest,
  																				AnalysisType::IDE_SolverTest,
  																				AnalysisType::MONO_Intra_SolverTest,
  																				AnalysisType::MONO_Inter_SolverTest };
  if (!Analyses.empty()) {
    ChosenAnalyses.clear();
  	for (auto& Analysis : Analyses) {
  		if (Analysis == "ifds_uninit")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_UninitializedVariables);
  		else if (Analysis == "ifds_taint")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_TaintAnalysis);
			else if (Analysis == "ifds_const")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_ConstnessAnalysis);
  		else if (Analysis == "ifds_type")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_TypeAnalysis);
  		else if (Analysis == "ide_taint")
  			ChosenAnalyses.push_back(AnalysisType::IDE_TaintAnalysis);
  		else if (Analysis == "ifds_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::IFDS_SolverTest);
  		else if (Analysis == "ide_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::IDE_SolverTest);
  		else if (Analysis == "mono_intra_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::MONO_Intra_SolverTest);
  		else if (Analysis == "mono_inter_solvertest")
  			ChosenAnalyses.push_back(AnalysisType::MONO_Inter_SolverTest);
  		else if (Analysis == "none") {
  			ChosenAnalyses.clear();
  			ChosenAnalyses.push_back(AnalysisType::None);
  		} else {
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
  		AnalysisController Controller(IRDB, ChosenAnalyses, WPAMode, Mem2Reg, PrintEdgeRecorder);
  	} else {
  		if (!(bfs::exists(ProjectPath) && bfs::is_directory(ProjectPath))) {
  			cerr << "error: '" << ProjectPath << "' is not a valid directory, abort\n";
  			return 1;
  		}
  		// perform a little trick to make OptionsParser only responsible for the project sources
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
  		AnalysisController Controller(IRDB, ChosenAnalyses, WPAMode, Mem2Reg);
  	}
  } else {
  	cerr << "error: expected at least the specification of parameter 'module' or 'project'\n"
  					"note: analysis can only be a single module OR whole project analysis\n"
  					"use --help to show help message, abort\n";
  	return 1;
  }
  llvm::llvm_shutdown();
  cout << "... shutdown analysis ...\n";
  return 0;
}
