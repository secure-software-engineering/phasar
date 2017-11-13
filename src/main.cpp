#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "analysis/AnalysisController.hh"
#include "analysis/passes/GeneralStatisticsPass.hh"
#include "analysis/passes/ValueAnnotationPass.hh"
#include "db/ProjectIRCompiledDB.hh"
#include "utils/Configuration.hh"
#include "utils/Logger.hh"
#include "utils/utils.hh"

// this project is pretty large at this point, to avoid
// confusion only use the 'using namespace' command for the STL
// all other scopes shall be used explicitly
using namespace std;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory("Static Analysis");
static llvm::cl::extrahelp CommonHelp(
    clang::tooling::CommonOptionsParser::HelpMessage);
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;
static const string MoreHelp =
#include "more_help.txt"
    ;

// // initialize the module passes ID's that we are using
char GeneralStatisticsPass::ID = 0;
char ValueAnnotationPass::ID = 13;

namespace boost {
void throw_exception(std::exception const &e) {}
}

// functions for parameter validation
void validateParamModule(const vector<string> &modules) {
  for (const auto &module : modules) {
    if (!(bfs::exists(module) && !bfs::is_directory(module))) {
      throw bpo::error_with_option_name("'" + module +
                                        "' is not a valid module");
    }
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
    if (StringToDataFlowAnalysisType.count(analysis) == 0) {
      throw bpo::error_with_option_name("'" + analysis +
                                        "' is not a valid data-flow analysis");
    }
  }
}

void validateParamPointerAnalysis(const string &pta) {
  if (StringToPointerAnalysisType.count(pta) == 0) {
    throw bpo::error_with_option_name("'" + pta +
                                      "' is not a valid pointer analysis");
  }
}

void validateParamCallGraphAnalysis(const string &cga) {
  if (StringToCallGraphAnalysisType.count(cga) == 0) {
    throw bpo::error_with_option_name("'" + cga +
                                      "' is not a valid call-graph analysis");
  }
}

void validateParamExport(const string &exp) {
  if (StringToExportType.count(exp) == 0) {
    throw bpo::error_with_option_name("'" + exp +
                                      "' is not a valid export parameter");
  }
}

void validateParamAnalysisPlugin(const string &plugin) {
  if (!(bfs::exists(plugin) && !bfs::is_directory(plugin) &&
        bfs::extension(plugin) == ".so")) {
    throw bpo::error_with_option_name("'" + plugin +
                                      "' is not a valid shared object library");
  }
}

void validateParamAnalysisInterface(const string &iface) {
  if (!AvailablePluginInterfaces.count(iface)) {
    throw bpo::error_with_option_name("'" + iface +
                                      "' is not a valid plugin interface");
  }
}

void validateParamConfig(const string &cfg) {
  if (!(bfs::exists(cfg) && !bfs::is_directory(cfg))) {
    throw bpo::error_with_option_name("'" + cfg + "' does not exist");
  }
}

void validateParamOutput(const string &filename) {
  if (bfs::exists(filename)) {
    if (bfs::is_directory(filename)) {
      throw bpo::error_with_option_name("'" + filename + "' is a directory");
    }
    throw bpo::error_with_option_name("'" + filename + "' already exists");
  }
}

void validateParamGraphID(const string &graphid) {
  // TODO perform some validation
}

template <typename T>
ostream &operator<<(ostream &os, const vector<T> &v) {
  copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
  return os;
}

int main(int argc, const char **argv) {
  // set-up the logger and get a reference to it
  initializeLogger(true);
  auto &lg = lg::get();
  // handling the command line parameters
  BOOST_LOG_SEV(lg, DEBUG) << "Set-up the command-line parameters";
  bpo::variables_map VariablesMap;
  try {
    string ConfigFile;
    // Declare a group of options that will be allowed only on command line
    bpo::options_description Generic(MoreHelp + "\n\nCommand-line options");
    // clang-format off
		Generic.add_options()
			("help,h", "Print help message")
		  ("config", bpo::value<string>(&ConfigFile)->notifier(validateParamConfig), "Path to the configuration file, options can be specified as 'parameter = option'");
    // clang-format on
    // Declare a group of options that will be allowed both on command line and
    // in config file
    bpo::options_description Config("Configuration file options");
    // clang-format off
    Config.add_options()
      ("graph_id,G", bpo::value<string>()->default_value("123456")->notifier(validateParamGraphID), "Graph Id used by the visulization framework")
			("function,f", bpo::value<string>(), "Function under analysis (a mangled function name)")
			("module,m", bpo::value<vector<string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamModule), "Path to the module(s) under analysis")
			("project,p", bpo::value<string>()->notifier(validateParamProject), "Path to the project under analysis")
			("data_flow_analysis,D", bpo::value<vector<string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamDataFlowAnalysis), "Analysis")
			("pointer_analysis,P", bpo::value<string>()->notifier(validateParamPointerAnalysis), "Points-to analysis (CFLSteens, CFLAnders)")
      ("callgraph_analysis,C", bpo::value<string>()->notifier(validateParamCallGraphAnalysis), "Call-graph analysis (CHA, RTA, DTA, VTA, OTF)")
      ("entry_points", bpo::value<vector<string>>()->multitoken()->zero_tokens()->composing(), "Entry point(s)")
			("classhierachy_analysis,H", bpo::value<bool>(), "Class-hierarchy analysis")
			("vtable_analysis,V", bpo::value<bool>(), "Virtual function table analysis")
			("statistical_analysis,S", bpo::value<bool>(), "Statistics")
			("export,E", bpo::value<string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("wpa,W", bpo::value<bool>()->default_value(1), "WPA mode (1 or 0)")
			("mem2reg,M", bpo::value<bool>()->default_value(1), "Promote memory to register pass (1 or 0)")
			("printedgerec,R", bpo::value<bool>()->default_value(0), "Print exploded-super-graph edge recorder (1 or 0)")
			("analysis_plugin", bpo::value<string>()->notifier(validateParamAnalysisPlugin), "Analysis plugin (absolute path to the shared object file)")
			("analysis_interface", bpo::value<string>()->notifier(validateParamAnalysisInterface), "Interface to be used for the plugin")
      ("output,O", bpo::value<string>()->notifier(validateParamOutput)->default_value("results.json"), "Filename for the results");
      ;
    // clang-format on
    bpo::options_description CmdlineOptions;
    CmdlineOptions.add(Generic).add(Config);
    bpo::options_description ConfigFileOptions;
    ConfigFileOptions.add(Config);
    bpo::options_description Visible("Allowed options");
    Visible.add(Generic).add(Config);
    bpo::store(
        bpo::command_line_parser(argc, argv).options(CmdlineOptions).run(),
        VariablesMap);
    bpo::notify(VariablesMap);
    ifstream ifs(ConfigFile.c_str());
    if (!ifs) {
      BOOST_LOG_SEV(lg, INFO) << "No configuration file is used.";
    } else {
      BOOST_LOG_SEV(lg, INFO) << "Using configuration file: " << ConfigFile;
      bpo::store(bpo::parse_config_file(ifs, ConfigFileOptions), VariablesMap);
      bpo::notify(VariablesMap);
    }
    // check if we have anything at all or a call for help
    if (argc < 2 || VariablesMap.count("help")) {
      cout << Visible << '\n';
      return 0;
    }
    BOOST_LOG_SEV(lg, INFO) << "Program options have been successfully parsed.";
    bl::core::get()->flush();
    if (VariablesMap.count("config")) {
      cout << "Configuration fille: " << VariablesMap["config"].as<string>()
           << '\n';
    }
    if (VariablesMap.count("graph_id")) {
      cout << "Graph ID: " << VariablesMap["graph_id"].as<string>() << '\n';
    }
    if (VariablesMap.count("function")) {
      cout << "Function: " << VariablesMap["function"].as<string>() << '\n';
    }
    if (VariablesMap.count("module")) {
      cout << "Module(s): " << VariablesMap["module"].as<vector<string>>()
           << '\n';
    }
    if (VariablesMap.count("project")) {
      cout << "Project: " << VariablesMap["project"].as<string>() << '\n';
    }
    if (VariablesMap.count("data_flow_analysis")) {
      cout << "Data-flow analysis: "
           << VariablesMap["data_flow_analysis"].as<vector<string>>() << '\n';
    }
    if (VariablesMap.count("pointer_analysis")) {
      cout << "Pointer analysis: "
           << VariablesMap["pointer_analysis"].as<string>() << '\n';
    }
    if (VariablesMap.count("callgraph_analysis")) {
      cout << "Callgraph analysis: "
           << VariablesMap["callgraph_analysis"].as<string>() << '\n';
    }
    if (VariablesMap.count("entry_points")) {
      cout << "Entry points: "
           << VariablesMap["entry_points"].as<vector<string>>() << '\n';
    }
    if (VariablesMap.count("classhierarchy_analysis")) {
      cout << "Classhierarchy analysis: "
           << VariablesMap["classhierarchy_analysis"].as<bool>() << '\n';
    }
    if (VariablesMap.count("vtable_analysis")) {
      cout << "Vtable analysis: " << VariablesMap["vtable_analysis"].as<bool>()
           << '\n';
    }
    if (VariablesMap.count("statistical_analysis")) {
      cout << "Statistical analysis: "
           << VariablesMap["statistical_analysis"].as<bool>() << '\n';
    }
    if (VariablesMap.count("export")) {
      cout << "Export: " << VariablesMap["export"].as<string>() << '\n';
    }
    if (VariablesMap.count("wpa")) {
      cout << "WPA: " << VariablesMap["wpa"].as<bool>() << '\n';
    }
    if (VariablesMap.count("mem2reg")) {
      cout << "Mem2reg: " << VariablesMap["mem2reg"].as<bool>() << '\n';
    }
    if (VariablesMap.count("printedgerec")) {
      cout << "Print edge recorder: " << VariablesMap["printedgerec"].as<bool>()
           << '\n';
    }
    if (VariablesMap.count("analysis_plugin")) {
      cout << "Analysis plugin: "
           << VariablesMap["analysis_plugin"].as<string>() << '\n';
    }
    if (VariablesMap.count("analysis_interface")) {
      cout << "Analysis interface: "
           << VariablesMap["analysis_interface"].as<string>() << '\n';
    }
    if (VariablesMap.count("output")) {
      cout << "Output: " << VariablesMap["output"].as<string>() << '\n';
    }
    BOOST_LOG_SEV(lg, INFO) << "Check program options for logical errors.";
    // validate the logic of the command-line arguments
    if (VariablesMap.count("project") == VariablesMap.count("module")) {
      cerr << "Either a project OR a module must be specified for an "
              "analysis.\n";
      return 1;
    }
    // validate plugin concept, if an analysis plugin is chosen, the plugin
    // interface and plugin itself must also be specified.
    if (VariablesMap.count("data_flow_analysis")) {
      if (find(VariablesMap["data_flow_analysis"].as<vector<string>>().begin(),
               VariablesMap["data_flow_analysis"].as<vector<string>>().end(),
               "plugin") !=
              VariablesMap["data_flow_analysis"].as<vector<string>>().end() &&
          (!VariablesMap.count("analysis_plugin") ||
           !VariablesMap.count("analysis_interface"))) {
        cerr << "If an analysis plugin is chosen, the plugin interface and "
                "plugin itself must also be specified.\n";
        return 1;
      }
    }
  } catch (const bpo::error &e) {
    cerr << "error: could not parse program options\n"
            "message: "
         << e.what() << ", abort\n";
    return 1;
  }
  vector<DataFlowAnalysisType> ChosenDataFlowAnalyses = {
      DataFlowAnalysisType::None};
  if (VariablesMap.count("data_flow_analysis")) {
    ChosenDataFlowAnalyses.clear();
    for (auto &DataFlowAnalysis :
         VariablesMap["data_flow_analysis"].as<vector<string>>()) {
      if (StringToDataFlowAnalysisType.count(DataFlowAnalysis)) {
        ChosenDataFlowAnalyses.push_back(
            StringToDataFlowAnalysisType.at(DataFlowAnalysis));
      }
    }
  }
  AnalysisController Controller(
      [&lg,&VariablesMap](bool usingModules) {
        BOOST_LOG_SEV(lg, INFO) << "Set-up IR database.";
        if (usingModules) {
          vector<const char *> CompileArgs;
          ProjectIRCompiledDB IRDB(VariablesMap["module"].as<vector<string>>(),
                                   CompileArgs);
          return IRDB;
        } else {
          // perform a little trick to make OptionsParser only responsible for
          // the project sources
          int OnlyTakeCareOfSources = 2;
          const char *ProjectSources =
              VariablesMap["project"].as<string>().c_str();
          const char *DummyProgName = "not_important";
          const char *DummyArgs[] = {DummyProgName, ProjectSources};
          clang::tooling::CommonOptionsParser OptionsParser(
              OnlyTakeCareOfSources, DummyArgs, StaticAnalysisCategory,
              OccurrencesFlag);
          clang::tooling::CompilationDatabase &CompileDB =
              OptionsParser.getCompilations();
          ProjectIRCompiledDB IRDB(CompileDB);
          return IRDB;
        }
      }(VariablesMap.count("module")),
      ChosenDataFlowAnalyses, VariablesMap["wpa"].as<bool>(),
      VariablesMap["mem2reg"].as<bool>(),
      VariablesMap["printedgerec"].as<bool>(), VariablesMap["graph_id"].as<string>());
  BOOST_LOG_SEV(lg, INFO) << "Write results to file";
  // Controller.writeResults(VariablesMap["output"].as<string>());
  // free all resources handled by llvm
  llvm::llvm_shutdown();
  BOOST_LOG_SEV(lg, INFO) << "Shutdown llvm and the analysis framework.";
  // flush the log core at last (performs flush() on all registered sinks)
  bl::core::get()->flush();
  return 0;
}
