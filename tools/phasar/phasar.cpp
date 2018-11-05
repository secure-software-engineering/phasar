/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/CommandLine.h>

#include <phasar/Config/Configuration.h>
#include <phasar/Controller/AnalysisController.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarClang/ClangController.h>
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/Passes/GeneralStatisticsPass.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonoProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonoProblemPlugin.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
#include <phasar/Utils/EnumFlags.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
using namespace std;
using namespace psr;

// setup programs command line options (via Clang)
static llvm::cl::OptionCategory StaticAnalysisCategory("Static Analysis");
static llvm::cl::extrahelp CommonHelp(
    clang::tooling::CommonOptionsParser::HelpMessage);
llvm::cl::NumOccurrencesFlag OccurrencesFlag = llvm::cl::Optional;

static const string MORE_PHASAR_LLVM_HELP(
#include "../phasar-llvm_more_help.txt"
    );
static const string MORE_PHASAR_CLANG_HELP("");

namespace boost {
void throw_exception(std::exception const &e) {}
}  // namespace boost

// functions for parameter validation
void validateParamModule(const std::vector<std::string> &modules) {
  for (const auto &module : modules) {
    if (!(bfs::exists(module) && !bfs::is_directory(module))) {
      throw bpo::error_with_option_name("'" + module +
                                        "' is not a valid module");
    }
  }
}

void validateParamProject(const std::string &project) {
  if (!(bfs::exists(project) && bfs::is_directory(project) &&
        bfs::exists(bfs::path(project) / CompileCommandsJson))) {
    throw bpo::error_with_option_name("'" + project +
                                      "' is not a valid project");
  }
}

void validateParamDataFlowAnalysis(const std::vector<std::string> &dfa) {
  for (const auto &analysis : dfa) {
    if (StringToDataFlowAnalysisType.count(analysis) == 0) {
      throw bpo::error_with_option_name("'" + analysis +
                                        "' is not a valid data-flow analysis");
    }
  }
}

void validateParamPointerAnalysis(const std::string &pta) {
  if (StringToPointerAnalysisType.count(pta) == 0) {
    throw bpo::error_with_option_name("'" + pta +
                                      "' is not a valid pointer analysis");
  }
}

void validateParamCallGraphAnalysis(const std::string &cga) {
  if (StringToCallGraphAnalysisType.count(cga) == 0) {
    throw bpo::error_with_option_name("'" + cga +
                                      "' is not a valid call-graph analysis");
  }
}

void validateParamExport(const std::string &exp) {
  if (StringToExportType.count(exp) == 0) {
    throw bpo::error_with_option_name("'" + exp +
                                      "' is not a valid export parameter");
  }
}

void validateParamAnalysisPlugin(const std::vector<std::string> &plugins) {
  for (const auto &plugin : plugins) {
    if (!bfs::exists(plugin) && !bfs::is_directory(plugin) &&
        bfs::extension(plugin) == ".so") {
      throw bpo::error_with_option_name(
          "'" + plugin + "' is not a valid shared object library");
    }
  }
}

void validateParamICFGPlugin(const std::string &plugin) {
  if (!bfs::exists(plugin) && !bfs::is_directory(plugin) &&
      bfs::extension(plugin) == ".so") {
    throw bpo::error_with_option_name("'" + plugin +
                                      "' is not a valid shared object library");
  }
  if (VariablesMap.count("callgraph-analysis")) {
    throw bpo::error_with_option_name(
        "Cannot choose a built-in callgraph AND "
        "a plug-in for callgraph construction.");
  }
  if (VariablesMap.count("wpa") && !VariablesMap["wpa"].as<bool>()) {
    throw bpo::error_with_option_name(
        "Plug-in for callgraph construction can only be used in 'wpa' mode.");
  }
}

void validateParamConfig(const std::string &cfg) {
  if (!(bfs::exists(cfg) && !bfs::is_directory(cfg))) {
    throw bpo::error_with_option_name("'" + cfg + "' does not exist");
  }
}

void validateParamOutput(const std::string &filename) {
  if (bfs::exists(filename)) {
    if (bfs::is_directory(filename)) {
      throw bpo::error_with_option_name("'" + filename + "' is a directory");
    }
  }
}

void validateParamGraphID(const std::string &graphID) {
  // TODO perform some validation
}

void validateParamProjectID(const std::string &projectID) {
  // TODO perform some validation
}

void validateParamMode(const std::string &mode) {
  if (mode != "phasarLLVM" && mode != "phasarClang") {
    throw bpo::error_with_option_name(
        "Phasar operation mode must be either 'phasarLLVM' or 'phasarClang', "
        "but is: '" +
        mode + "'");
  }
}

template <typename T>
ostream &operator<<(ostream &os, const std::vector<T> &v) {
  copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
  return os;
}

int main(int argc, const char **argv) {
  PAMM_GET_INSTANCE;
  START_TIMER("Phasar Runtime", PAMM_SEVERITY_LEVEL::Core);
  // set-up the logger and get a reference to it
  initializeLogger(false);
  auto &lg = lg::get();
  // handling the command line parameters
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Set-up the command-line parameters");
  // Here we start creating Phasars top level operation mode:
  // Based on the mode, we delegate to the further subtools and their
  // corresponding argument parsers.
  bpo::variables_map ModeMap;
  bpo::options_description PhasarMode("Phasar operation mode");
  // clang-format off
    PhasarMode.add_options()
      ("mode", bpo::value<std::string>()->notifier(validateParamMode), "Set the operation mode: 'phasarLLVM' or 'phasarClang'");
  // clang-format on
  bpo::options_description ModeOptions;
  ModeOptions.add(PhasarMode);
  try {
    // Just parse the mode options and ignore everythin else, since the other
    // parsers deal with it
    bpo::store(bpo::command_line_parser(argc, argv)
                   .options(ModeOptions)
                   .allow_unregistered()
                   .run(),
               ModeMap);
    bpo::notify(ModeMap);
  } catch (const bpo::error &e) {
    std::cerr << "error: could not parse program options\n"
                 "message: "
              << e.what() << ", abort\n";
    return 1;
  }
  // Make sure an operation mode has been set
  // If it has not been set explicitly by the user, then use
  // phasarLLVM as default.
  if (!ModeMap.count("mode")) {
    ModeMap.insert(
        make_pair("mode", bpo::variable_value(string("phasarLLVM"), false)));
  }
  // Next we can check what operation mode was chosen and resume accordingly:
  if (ModeMap["mode"].as<std::string>() == "phasarLLVM") {
    // --- LLVM mode ---
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "Chosen operation mode: 'phasarLLVM'");
    try {
      std::string ConfigFile;
      // Declare a group of options that will be allowed only on command line
      bpo::options_description Generic("Command-line options");
      // clang-format off
		Generic.add_options()
			("help,h", "Print help message")
      ("more_help", "Print more help")
		  ("config", bpo::value<std::string>(&ConfigFile)->notifier(validateParamConfig), "Path to the configuration file, options can be specified as 'parameter = option'")
      ("silent", "Suppress any non-result output");
      // clang-format on
      // Declare a group of options that will be allowed both on command line
      // and in config file
      bpo::options_description Config("Configuration file options");
      // clang-format off
    Config.add_options()
			("function,f", bpo::value<std::string>(), "Function under analysis (a mangled function name)")
			("module,m", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamModule), "Path to the module(s) under analysis")
			("project,p", bpo::value<std::string>()->notifier(validateParamProject), "Path to the project under analysis")
      ("entry-points,E", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Set the entry point(s) to be used")
      ("output,O", bpo::value<std::string>()->notifier(validateParamOutput)->default_value("results.json"), "Filename for the results")
			("data-flow-analysis,D", bpo::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(validateParamDataFlowAnalysis), "Set the analysis to be run")
			("pointer-analysis,P", bpo::value<std::string>()->notifier(validateParamPointerAnalysis), "Set the points-to analysis to be used (CFLSteens, CFLAnders)")
      ("callgraph-analysis,C", bpo::value<std::string>()->notifier(validateParamCallGraphAnalysis), "Set the call-graph algorithm to be used (CHA, RTA, DTA, VTA, OTF)")
			("classhierachy-analysis,H", bpo::value<bool>(), "Class-hierarchy analysis")
			("vtable-analysis,V", bpo::value<bool>(), "Virtual function table analysis")
			("statistical-analysis,S", bpo::value<bool>(), "Statistics")
			//("export,E", bpo::value<std::string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("wpa,W", bpo::value<bool>()->default_value(1), "Whole-program analysis mode (1 or 0)")
			("mem2reg,M", bpo::value<bool>()->default_value(1), "Promote memory to register pass (1 or 0)")
			("printedgerec,R", bpo::value<bool>()->default_value(0), "Print exploded-super-graph edge recorder (1 or 0)")
      #ifdef PHASAR_PLUGINS_ENABLED
			("analysis-plugin", bpo::value<std::vector<std::string>>()->notifier(validateParamAnalysisPlugin), "Analysis plugin(s) (absolute path to the shared object file(s))")
      ("callgraph-plugin", bpo::value<std::string>()->notifier(validateParamICFGPlugin), "ICFG plugin (absolute path to the shared object file)")
      #endif
      ("project-id", bpo::value<std::string>()->default_value("myphasarproject")->notifier(validateParamProjectID), "Project Id used for the database")
      ("graph-id", bpo::value<std::string>()->default_value("123456")->notifier(validateParamGraphID), "Graph Id used by the visualization framework")
      ("pamm-out", bpo::value<std::string>()->notifier(validateParamOutput)->default_value("PAMM_data.json"), "Filename for PAMM's gathered data");
      // clang-format on
      bpo::options_description CmdlineOptions;
      CmdlineOptions.add(PhasarMode).add(Generic).add(Config);
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
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                      << "No configuration file is used.");
      } else {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Using configuration file: "
                                              << ConfigFile);
        bpo::store(bpo::parse_config_file(ifs, ConfigFileOptions),
                   VariablesMap);
        bpo::notify(VariablesMap);
      }

      // Vanity header
      if (!VariablesMap.count("silent")) {
        std::cout << PhasarVersion
                  << "\n"
                     "A LLVM-based static analysis framework\n\n";
      }
      // check if we have anything at all or a call for help
      if ((argc < 3 || VariablesMap.count("help")) &&
          !VariablesMap.count("silent")) {
        std::cout << Visible << '\n';
        return 0;
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Program options have been successfully parsed.");
      bl::core::get()->flush();

      if (!VariablesMap.count("silent")) {
        // Print current configuration
        if (VariablesMap.count("more_help")) {
          if (!VariablesMap.count("help")) {
            std::cout << Visible << '\n';
          }
          std::cout << MORE_PHASAR_LLVM_HELP << '\n';
          return 0;
        }
        std::cout << "--- Configuration ---\n";
        if (VariablesMap.count("config")) {
          std::cout << "Configuration file: "
                    << VariablesMap["config"].as<std::string>() << '\n';
        }
        if (VariablesMap.count("project-id")) {
          std::cout << "Project ID: "
                    << VariablesMap["project-id"].as<std::string>() << '\n';
        }
        if (VariablesMap.count("graph-id")) {
          std::cout << "Graph ID: "
                    << VariablesMap["graph-id"].as<std::string>() << '\n';
        }
        if (VariablesMap.count("function")) {
          std::cout << "Function: "
                    << VariablesMap["function"].as<std::string>() << '\n';
        }
        if (VariablesMap.count("module")) {
          std::cout << "Module(s): "
                    << VariablesMap["module"].as<std::vector<std::string>>()
                    << '\n';
        }
        if (VariablesMap.count("project")) {
          std::cout << "Project: " << VariablesMap["project"].as<std::string>()
                    << '\n';
        }
        if (VariablesMap.count("data-flow-analysis")) {
          std::cout << "Data-flow analysis: "
                    << VariablesMap["data-flow-analysis"]
                           .as<std::vector<std::string>>()
                    << '\n';
        }
        if (VariablesMap.count("pointer-analysis")) {
          std::cout << "Pointer analysis: "
                    << VariablesMap["pointer-analysis"].as<std::string>()
                    << '\n';
        }
        if (VariablesMap.count("callgraph-analysis")) {
          std::cout << "Callgraph analysis: "
                    << VariablesMap["callgraph-analysis"].as<std::string>()
                    << '\n';
        }
        if (VariablesMap.count("entry-points")) {
          std::cout
              << "Entry points: "
              << VariablesMap["entry-points"].as<std::vector<std::string>>()
              << '\n';
        }
        if (VariablesMap.count("classhierarchy_analysis")) {
          std::cout << "Classhierarchy analysis: "
                    << VariablesMap["classhierarchy_analysis"].as<bool>()
                    << '\n';
        }
        if (VariablesMap.count("vtable-analysis")) {
          std::cout << "Vtable analysis: "
                    << VariablesMap["vtable-analysis"].as<bool>() << '\n';
        }
        if (VariablesMap.count("statistical-analysis")) {
          std::cout << "Statistical analysis: "
                    << VariablesMap["statistical-analysis"].as<bool>() << '\n';
        }
        if (VariablesMap.count("export")) {
          std::cout << "Export: " << VariablesMap["export"].as<std::string>()
                    << '\n';
        }
        if (VariablesMap.count("wpa")) {
          std::cout << "WPA: " << VariablesMap["wpa"].as<bool>() << '\n';
        }
        if (VariablesMap.count("mem2reg")) {
          std::cout << "Mem2reg: " << VariablesMap["mem2reg"].as<bool>()
                    << '\n';
        }
        if (VariablesMap.count("printedgerec")) {
          std::cout << "Print edge recorder: "
                    << VariablesMap["printedgerec"].as<bool>() << '\n';
        }
        if (VariablesMap.count("analysis-plugin")) {
          std::cout << "Analysis plugin(s): \n";
          for (const auto &analysis_plugin :
               VariablesMap["analysis-plugin"].as<std::vector<std::string>>()) {
            std::cout << analysis_plugin << '\n';
          }
        }
        if (VariablesMap.count("output")) {
          std::cout << "Output: " << VariablesMap["output"].as<std::string>()
                    << '\n';
        }
        if (VariablesMap.count("output-pamm")) {
          std::cout << "Output PAMM: " << VariablesMap["output-pamm"].as<std::string>()
                    << '\n';
        }
      } else {
        setLoggerFilterLevel(INFO);
      }

      // Validation
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Check program options for logical errors.");
      // validate the logic of the command-line arguments
      if (VariablesMap.count("project") == VariablesMap.count("module")) {
        std::cerr << "Either a project OR a module must be specified for an "
                     "analysis.\n";
        return 1;
      }

      // Plugin Validation
      if (VariablesMap.count("data-flow-analysis")) {
        if (find(VariablesMap["data-flow-analysis"]
                     .as<std::vector<std::string>>()
                     .begin(),
                 VariablesMap["data-flow-analysis"]
                     .as<std::vector<std::string>>()
                     .end(),
                 "plugin") !=
                VariablesMap["data-flow-analysis"]
                    .as<std::vector<std::string>>()
                    .end() &&
            (!VariablesMap.count("analysis-plugin"))) {
          std::cerr
              << "If an analysis plugin is chosen, the plugin itself must also "
                 "be specified.\n";
          return 1;
        }
      }
    } catch (const bpo::error &e) {
      std::cerr << "error: could not parse program options\n"
                   "message: "
                << e.what() << ", abort\n";
      return 1;
    }

    // Set chosen dfa
    std::vector<DataFlowAnalysisType> ChosenDataFlowAnalyses = {
        DataFlowAnalysisType::None};
    if (VariablesMap.count("data-flow-analysis")) {
      ChosenDataFlowAnalyses.clear();
      for (auto &DataFlowAnalysis :
           VariablesMap["data-flow-analysis"].as<std::vector<std::string>>()) {
        if (StringToDataFlowAnalysisType.count(DataFlowAnalysis)) {
          ChosenDataFlowAnalyses.push_back(
              StringToDataFlowAnalysisType.at(DataFlowAnalysis));
        }
      }
    }

#ifdef PHASAR_PLUGINS_ENABLED
    // Check if user has specified an analysis plugin
    if (!IDETabulationProblemPluginFactory.empty() ||
        !IFDSTabulationProblemPluginFactory.empty() ||
        !IntraMonoProblemPluginFactory.empty() ||
        !InterMonoProblemPluginFactory.empty()) {
      ChosenDataFlowAnalyses.push_back(DataFlowAnalysisType::Plugin);
    }
#endif

    // At this point we have set-up all the parameters and can start the actual
    // analyses that have been choosen.
    AnalysisController Controller(
        [&lg](bool usingModules) {
          PAMM_GET_INSTANCE;
          START_TIMER("IRDB Construction", PAMM_SEVERITY_LEVEL::Full);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Set-up IR database.");
          IRDBOptions Opt = IRDBOptions::NONE;
          if (VariablesMap["wpa"].as<bool>()) {
            Opt |= IRDBOptions::WPA;
          }
          if (VariablesMap["mem2reg"].as<bool>()) {
            Opt |= IRDBOptions::MEM2REG;
          }
          if (usingModules) {
            ProjectIRDB IRDB(
                VariablesMap["module"].as<std::vector<std::string>>(), Opt);
            STOP_TIMER("IRDB Construction", PAMM_SEVERITY_LEVEL::Full);
            return IRDB;
          } else {
            // perform a little trick to make OptionsParser only responsible for
            // the project sources
            int OnlyTakeCareOfSources = 2;
            const char *ProjectSources =
                VariablesMap["project"].as<std::string>().c_str();
            const char *DummyProgName = "not_important";
            const char *DummyArgs[] = {DummyProgName, ProjectSources};
            clang::tooling::CommonOptionsParser OptionsParser(
                OnlyTakeCareOfSources, DummyArgs, StaticAnalysisCategory,
                OccurrencesFlag);
            clang::tooling::CompilationDatabase &CompileDB =
                OptionsParser.getCompilations();
            ProjectIRDB IRDB(CompileDB, Opt);
            STOP_TIMER("IRDB Construction", PAMM_SEVERITY_LEVEL::Full);
            return IRDB;
          }
        }(VariablesMap.count("module")),
        ChosenDataFlowAnalyses, VariablesMap["wpa"].as<bool>(),
        VariablesMap["printedgerec"].as<bool>(),
        VariablesMap["graph-id"].as<std::string>());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Write results to file");
    Controller.writeResults(VariablesMap["output"].as<std::string>());
  } else {
    // -- Clang mode ---
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "Chosen operation mode: 'phasarClang'");
    std::string ConfigFile;
    // Declare a group of options that will be allowed only on command line
    bpo::options_description Generic("Command-line options");
    // clang-format off
    Generic.add_options()
    	("help,h", "Print help message")
      ("config", bpo::value<std::string>(&ConfigFile)->notifier(validateParamConfig), "Path to the configuration file, options can be specified as 'parameter = option'");
    // clang-format on
    // Declare a group of options that will be allowed both on command line and
    // in config file
    bpo::options_description Config("Configuration file options");
    // clang-format off
    Config.add_options()
    	("project,p", bpo::value<std::string>()->notifier(validateParamProject), "Path to the project under analysis");
    // clang-format on
    bpo::options_description CmdlineOptions;
    CmdlineOptions.add(PhasarMode).add(Generic).add(Config);
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
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "No configuration file is used.");
    } else {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Using configuration file: "
                                            << ConfigFile);
      bpo::store(bpo::parse_config_file(ifs, ConfigFileOptions), VariablesMap);
      bpo::notify(VariablesMap);
    }
    // check if we have anything at all or a call for help
    if (argc < 3 || VariablesMap.count("help")) {
      std::cout << Visible << '\n';
      return 0;
    }
    // Print what has been parsed
    if (VariablesMap.count("project")) {
      std::cout << "Project: " << VariablesMap["project"].as<std::string>()
                << '\n';
    }
    // Bring Clang source-to-source transformation to life
    if (VariablesMap.count("project")) {
      int OnlyTakeCareOfSources = 2;
      const char *ProjectSources =
          VariablesMap["project"].as<std::string>().c_str();
      const char *DummyProgName = "not_important";
      const char *DummyArgs[] = {DummyProgName, ProjectSources};
      clang::tooling::CommonOptionsParser OptionsParser(
          OnlyTakeCareOfSources, DummyArgs, StaticAnalysisCategory,
          OccurrencesFlag);
      // At this point we have set-up all the parameters and can start the
      // actual
      // analyses that have been choosen.
      ClangController CC(OptionsParser);
    }
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Shutdown llvm and the analysis framework.");
  // free all resources handled by llvm
  llvm::llvm_shutdown();
  // flush the log core at last (performs flush() on all registered sinks)
  bl::core::get()->flush();
  STOP_TIMER("Phasar Runtime", PAMM_SEVERITY_LEVEL::Core);
  // PRINT_MEASURED_DATA(std::cout);
  EXPORT_MEASURED_DATA(VariablesMap["pamm-out"].as<std::string>());
  return 0;
}
