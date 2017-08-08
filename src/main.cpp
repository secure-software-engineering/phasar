#include "analysis/AnalysisController.hh"
#include "analysis/passes/GeneralStatisticsPass.hh"
#include "analysis/passes/ValueAnnotationPass.hh"
#include "db/ProjectIRCompiledDB.hh"
#include "utils/Configuration.hh"
#include "utils/Logger.hh"
#include "utils/utils.hh"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <stdexcept>
#include <string>
#include <vector>

// this project is pretty large at this point, to avoid
// confusion only use the 'using namespace' command for the STL
// all other scopes shall be used explicitly
using namespace std;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory("Static Analysis");
static llvm::cl::extrahelp
    CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;
static const string MoreHelp =
#include "more_help.txt"
    ;

// initialize the module passes ID's that we are using
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;

namespace boost {
void throw_exception(std::exception const &e) {}
}

// functions for parameter validation
void validateParamModule(const string &module) {
  if (!(bfs::exists(module) && !bfs::is_directory(module))) {
    throw bpo::error_with_option_name("'" + module + "' is not a valid module");
  }
}

void validateParamProject(const string &project) {
  if (!(bfs::exists(project) && bfs::is_directory(project) &&
        bfs::exists(bfs::path(project) / CompileCommandsJson))) {
    throw bpo::error_with_option_name("'" + project +
                                      "' is not a valid project");
  }
}

void validateParamDataFlowAnalysis(const vector<string> &dfa) {
  for (const auto &analysis : dfa) {
    if (DataFlowAnalysisTypeMap.count(analysis) == 0) {
      throw bpo::error_with_option_name("'" + analysis + 
                                        "' is not a valid data-flow analysis");
    }
  }
}

void validateParamPointerAnalysis(const string &pta) {
  if (PointerAnalysisTypeMap.count(pta) == 0) {
    throw bpo::error_with_option_name("'" + pta + 
                                      "' is not a valid pointer analysis");
  }
}

void validateParamCallGraphAnalysis(const string &cga) {
  if (CallGraphAnalysisTypeMap.count(cga) == 0) {
    throw bpo::error_with_option_name("'" + cga +
                                      "' is not a valid call-graph analysis");
  }
}

void validateParamExport(const string &exp) {
  if (ExportTypeMap.count(exp) == 0) {
    throw bpo::error_with_option_name("'" + exp +
                                      "' is not a valid export parameter");
  }
}

void validateParamAnalysisPlugin(const string &plugin) {
  if (!(bfs::exists(plugin) && !bfs::is_directory(plugin) && bfs::extension(plugin) == ".so")) {
    throw bpo::error_with_option_name("'" + plugin +
                                      "' is not a valid shared object library");
  }
}

void validateParamAnalysisInterface(const string &iface) {
  // TODO implement once the common plug-in interfaces are specified!
}

void validateParamConfig(const string &cfg) {
  if (!(bfs::exists(cfg) && !bfs::is_directory(cfg))) {
    throw bpo::error_with_option_name("'" + cfg + "' does not exist");
  }
}

int main(int argc, const char **argv) {
  // set-up the logger and get a reference to it
  initializeLogger(true);
  auto& lg = lg::get();
  // handling the command line parameters
  BOOST_LOG_SEV(lg, DEBUG) << "Set-up the command-line parameters";
  try {
    bpo::options_description GeneralOptions(MoreHelp +
                                            "\n\nCommand-line options");
    // clang-format off
		GeneralOptions.add_options()
			("help,h", "Print help message")
			("function,f", bpo::value<string>(), "Function under analysis (a mangled function name)")
			("module,m", bpo::value<string>()->notifier(validateParamModule), "Path to the module under analysis")
			("project,p", bpo::value<string>()->notifier(validateParamProject), "Path to the project under analysis")
			("data_flow_analysis,D", bpo::value<vector<string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamDataFlowAnalysis), "Analysis")
			("pointer_analysis,P", bpo::value<string>()->notifier(validateParamPointerAnalysis), "Points-to analysis (CFLSteens, CFLAnders)")
			("callgraph_analysis,C", bpo::value<string>()->notifier(validateParamCallGraphAnalysis), "Call-graph analysis (CHA, RTA, DTA, VTA, OTF)")
			("classhierachy_analysis,H", bpo::value<bool>(), "Class-hierarchy analysis")
			("vtable_analysis,V", bpo::value<bool>(), "Virtual function table analysis")
			("statistical_analysis,S", bpo::value<bool>(), "Statistics")
			("export,E", bpo::value<string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("wpa,W", bpo::value<bool>()->default_value(1), "WPA mode (1 or 0)")
			("mem2reg,M", bpo::value<bool>()->default_value(1), "Promote memory to register pass (1 or 0)")
			("printedgerec,R", bpo::value<bool>()->default_value(0), "Print exploded-super-graph edge recorder (1 or 0)")
			("analysis_plugin", bpo::value<string>()->notifier(validateParamAnalysisPlugin), "Analysis plugin (absolute path to the shared object file)")
			("analysis_iterface", bpo::value<string>()->notifier(validateParamAnalysisInterface), "Interface to be used for the plugin (TODO: yet to implement!)")
			("config", bpo::value<string>()->notifier(validateParamConfig), "Path to the configuration file, options can be specified as 'parameter = option'");
    // clang-format on
    bpo::options_description FileOptions("Configuration file options");
    // clang-format off
		FileOptions.add_options()
			("function,f", bpo::value<string>(), "Function under analysis (a mangled function name)")
			("module,m", bpo::value<string>()->notifier(validateParamModule), "Path to the module under analysis")
			("project,p", bpo::value<string>()->notifier(validateParamProject), "Path to the project under analysis")
			("data_flow_analysis,D", bpo::value<vector<string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamDataFlowAnalysis), "Analysis")
			("pointer_analysis,P", bpo::value<string>()->notifier(validateParamPointerAnalysis), "Points-to analysis (CFLSteens, CFLAnders)")
			("callgraph_analysis,C", bpo::value<string>()->notifier(validateParamCallGraphAnalysis), "Call-graph analysis (CHA, RTA, DTA, VTA, OTF)")
			("classhierachy_analysis,H", bpo::value<bool>(), "Class-hierarchy analysis")
			("vtable_analysis,V", bpo::value<bool>(), "Virtual function table analysis")
			("statistical_analysis,S", bpo::value<bool>(), "Statistics")
			("export,E", bpo::value<string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("wpa,W", bpo::value<bool>()->default_value(1), "WPA mode (1 or 0)")
			("mem2reg,M", bpo::value<bool>()->default_value(1), "Promote memory to register pass (1 or 0)")
			("printedgerec,R", bpo::value<bool>()->default_value(0), "Print exploded-super-graph edge recorder (1 or 0)")
			("analysis_plugin", bpo::value<string>()->notifier(validateParamAnalysisPlugin), "Analysis plugin (absolute path to the shared object file)")
			("analysis_iterface", bpo::value<string>()->notifier(validateParamAnalysisInterface), "Interface to be used for the plugin (TODO: yet to implement!)");
    // clang-format on
    bpo::store(bpo::parse_command_line(argc, argv, GeneralOptions), VariablesMap);
    if (VariablesMap.count("config")) {
      ifstream ifs(VariablesMap["config"].as<string>());
      if (ifs) {
        bpo::store(bpo::parse_config_file(ifs, FileOptions), VariablesMap);
      }
    }
    BOOST_LOG_SEV(lg, INFO) << "Program options have been successfully parsed.";
    BOOST_LOG_SEV(lg, INFO) << "Check program options for logical errors.";
    // validate command-line arguments using the validation functions
    bpo::notify(VariablesMap);
    // check if we have anything at all or a call for help
    if (argc < 2 || VariablesMap.count("help")) {
      cout << GeneralOptions << '\n';
      return 0;
    }
    // validate the logic of the command-line arguments
    if (VariablesMap.count("project") == VariablesMap.count("module")) {
      cerr << "Either a project OR a module must be specified for an "
              "analysis.\n";
      return 1;
    }
    if (VariablesMap.count("analysis_plugin") != VariablesMap.count("analysis_interface")) {
      cerr << "If an analysis plug-in is specified the corresponding interface "
              "must be specified as well\n";
      return 1;
    }
  } catch (const bpo::error &e) {
    cerr << "error: could not parse program options\n"
            "message: " << e.what() << ", abort\n";
    return 1;
  }

  vector<DataFlowAnalysisType> ChosenDataFlowAnalyses;
  if (VariablesMap.count("data_flow_analysis")) {
    for (auto &DataFlowAnalysis :
         VariablesMap["data_flow_analysis"].as<vector<string>>()) {
      if (DataFlowAnalysisTypeMap.count(DataFlowAnalysis)) {
        ChosenDataFlowAnalyses.push_back(DataFlowAnalysisTypeMap.at(DataFlowAnalysis));
      }
    }
  }
  BOOST_LOG_SEV(lg, INFO) << "Set-up IR database.";
  if (VariablesMap.count("module")) {
    vector<const char *> CompileArgs;
    ProjectIRCompiledDB IRDB(VariablesMap["module"].as<string>(), CompileArgs);
    AnalysisController Controller(
        IRDB, ChosenDataFlowAnalyses, VariablesMap["wpa"].as<bool>(),
        VariablesMap["mem2reg"].as<bool>(), VariablesMap["printedgerec"].as<bool>());
  } else {
    // perform a little trick to make OptionsParser only responsible for the
    // project sources
    int OnlyTakeCareOfSources = 2;
    const char *ProjectSources = VariablesMap["project"].as<string>().c_str();
    const char *DummyProgName = "not_important";
    const char *DummyArgs[] = {DummyProgName, ProjectSources};
    clang::tooling::CommonOptionsParser OptionsParser(
        OnlyTakeCareOfSources, DummyArgs, StaticAnalysisCategory,
        OccurrencesFlag);
    clang::tooling::CompilationDatabase &CompileDB =
        OptionsParser.getCompilations();
    ProjectIRCompiledDB IRDB(CompileDB);
    AnalysisController Controller(IRDB, ChosenDataFlowAnalyses,
                                  VariablesMap["wpa"].as<bool>(),
                                  VariablesMap["mem2reg"].as<bool>());
  }
  // free all resources handled by llvm
  llvm::llvm_shutdown();
  BOOST_LOG_SEV(lg, INFO) << "Shutdown llvm and the analysis framework.";
  // flush the log core at last (performs flush() on all registered sinks)
  bl::core::get()->flush();
  return 0;
}
