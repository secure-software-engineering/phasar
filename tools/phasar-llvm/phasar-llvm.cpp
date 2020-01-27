/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <set>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <phasar/Config/Configuration.h>
#include <phasar/Controller/AnalysisController.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>
#include <phasar/Utils/Logger.h>

using namespace psr;

constexpr char MoreHelp[] =
#include "../phasar-llvm_more_help.txt"
    ;

template <typename T> static std::set<T> vectorToSet(const std::vector<T> &v) {
  std::set<T> s;
  for_each(v.begin(), v.end(), [&s](T t) { s.insert(t); });
  return s;
}

void validateParamConfigFile(const std::string &Config) {
  if (!(boost::filesystem::exists(Config) &&
        !boost::filesystem::is_directory(Config))) {
    throw boost::program_options::error_with_option_name(
        "PhASAR configuration '" + Config + "' does not exist!");
  }
}

void validateParamModule(const std::vector<std::string> &Modules) {
  if (Modules.empty()) {
    throw boost::program_options::error_with_option_name(
        "At least one LLVM target module is required!");
  }
  for (const auto &Module : Modules) {
    boost::filesystem::path ModulePath(Module);
    if (!(boost::filesystem::exists(ModulePath) &&
          !boost::filesystem::is_directory(ModulePath) &&
          (ModulePath.extension() == ".ll" ||
           ModulePath.extension() == ".bc"))) {
      throw boost::program_options::error_with_option_name(
          "LLVM module '" + Module + "' does not exist!");
    }
  }
}

void validateParamOutput(const std::string &Output) {
  if (boost::filesystem::is_directory(Output)) {
    throw boost::program_options::error_with_option_name(
        "'" + Output + "' cannot be a directory!");
  }
}

void validateParamDataFlowAnalysis(const std::vector<std::string> &Analyses) {}

void validateParamAnalysisStrategy(const std::string &Strategy) {
  if (to_AnalysisStrategy(Strategy) == AnalysisStrategy::None) {
    throw boost::program_options::error_with_option_name(
        "Invalid analysis strategy '" + Strategy + "'!");
  }
}

void validateParamPointerAnalysis(const std::string &Analysis) {
  if (to_PointerAnalysisType(Analysis) == PointerAnalysisType::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Analysis + "' is not a valid pointer analysis!");
  }
}

void validateParamCallGraphAnalysis(const std::string &Analysis) {
  if (to_CallGraphAnalysisType(Analysis) == CallGraphAnalysisType::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Analysis + "' is not a valid call-graph analysis!");
  }
}

void validateParamAnalysisPlugin(const std::vector<std::string> &Plugins) {
  for (const auto &Plugin : Plugins) {
    boost::filesystem::path PluginPath(Plugin);
    if (!(boost::filesystem::exists(PluginPath) &&
          !boost::filesystem::is_directory(PluginPath) &&
          PluginPath.extension() == ".so")) {
      throw boost::program_options::error_with_option_name(
          "'" + Plugin + "' is not a valid data-flow analysis plugin!");
    }
  }
}

void validateParamICFGPlugin(const std::string &Plugin) {
  boost::filesystem::path PluginPath(Plugin);
  if (!(boost::filesystem::exists(PluginPath) &&
        !boost::filesystem::is_directory(PluginPath) &&
        PluginPath.extension() == ".so")) {
    throw boost::program_options::error_with_option_name(
        "ICFG plugin '" + Plugin + "' does not exist!");
  }
}

void validateParamAnalysisConfig(const std::vector<std::string> &Configs) {
  for (const auto &Config : Configs) {
    if (!(boost::filesystem::exists(Config) &&
          !boost::filesystem::is_directory(Config))) {
      throw boost::program_options::error_with_option_name(
          "Analysis configuration '" + Config + "' does not exist!");
    }
  }
}

int main(int argc, const char **argv) {
  // handling the command line parameters
  std::string ConfigFile;
  // Declare a group of options that will be allowed only on command line
  boost::program_options::options_description Generic("Command-line options");
  // clang-format off
		Generic.add_options()
      ("version,v","Print PhASAR version")
			("help,h", "Print help message")
      ("more-help", "Print more help")
		  ("config,c", boost::program_options::value<std::string>(&ConfigFile)->notifier(&validateParamConfigFile), "Path to the configuration file, options can be specified as 'parameter = option'")
      ("silent,s", "Suppress any non-result output");
  // clang-format on
  // Declare a group of options that will be allowed both on command line
  // and in config file
  boost::program_options::options_description Config(
      "Configuration file options");
  // clang-format off
    Config.add_options()
			("module,m", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamModule), "Path to the module(s) under analysis")
      ("entry-points,E", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Set the entry point(s) to be used")
      ("output,O", boost::program_options::value<std::string>()->notifier(&validateParamOutput)->default_value("results.json"), "Filename for the results")
			("data-flow-analysis,D", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamDataFlowAnalysis), "Set the analysis to be run")
			("analysis-strategy", boost::program_options::value<std::string>()->default_value("WPA")->notifier(&validateParamAnalysisStrategy))
      ("analysis-config", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamAnalysisConfig), "Set the analysis's configuration (if required)")
      ("pointer-analysis,P", boost::program_options::value<std::string>()->notifier(&validateParamPointerAnalysis), "Set the points-to analysis to be used (CFLSteens, CFLAnders)")
      ("callgraph-analysis,C", boost::program_options::value<std::string>()->notifier(&validateParamCallGraphAnalysis), "Set the call-graph algorithm to be used (CHA, RTA, DTA, VTA, OTF)")
			("classhierarchy-analysis,H", "Class-hierarchy analysis")
			("vtable-analysis,V", "Virtual function table analysis")
			("statistical-analysis,S", "Statistics")
			//("export,E", boost::program_options::value<std::string>()->notifier(validateParamExport), "Export mode (TODO: yet to implement!)")
			("mwa,M", "Enable Modulewise-program analysis mode")
			("mem2reg", "Promote memory to register pass")
			("printedgerec,R", "Print exploded-super-graph edge recorder")
      ("log,L", "Enable logging")
      ("emit-ir", "Emit preprocessed and annotated IR of analysis target")
      ("emit-raw-results", "Emit unprocessed/raw solver results")
      ("emit-esg-as-dot", "Emit the Exploded super-graph (ESG) as DOT graph")
      #ifdef PHASAR_PLUGINS_ENABLED
			("analysis-plugin", boost::program_options::value<std::vector<std::string>>()->notifier(&validateParamAnalysisPlugin), "Analysis plugin(s) (absolute path to the shared object file(s))")
      ("callgraph-plugin", boost::program_options::value<std::string>()->notifier(&validateParamICFGPlugin), "ICFG plugin (absolute path to the shared object file)")
      #endif
      ("project-id,I", boost::program_options::value<std::string>()->default_value("default-phasar-project"), "Project Id used for the database")
      ("pamm-out,A", boost::program_options::value<std::string>()->notifier(validateParamOutput)->default_value("PAMM_data.json"), "Filename for PAMM's gathered data");
  // clang-format on
  boost::program_options::options_description CmdlineOptions;
  CmdlineOptions.add(Generic).add(Config);
  boost::program_options::options_description ConfigFileOptions;
  ConfigFileOptions.add(Config);
  boost::program_options::options_description Visible("Allowed options");
  Visible.add(Generic).add(Config);
  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(CmdlineOptions)
            .run(),
        PhasarConfig::VariablesMap());
    boost::program_options::notify(PhasarConfig::VariablesMap());
  } catch (boost::program_options::error Err) {
    std::cerr << "Could not parse command-line arguments!\n"
              << "Error: " << Err.what() << '\n';
    return 1;
  }
  try {
    if (ConfigFile != "") {
      std::ifstream ifs(ConfigFile.c_str());
      if (!ifs) {
      } else {
        boost::program_options::store(
            boost::program_options::parse_config_file(ifs, ConfigFileOptions),
            PhasarConfig::VariablesMap());
        boost::program_options::notify(PhasarConfig::VariablesMap());
      }
    }
  } catch (boost::program_options::error Err) {
    std::cerr << "Could not parse configuration file!\n"
              << "Error: " << Err.what() << '\n';
    return 1;
  }
  initializeLogger(PhasarConfig::VariablesMap().count("log"));
  // print PhASER version
  if (PhasarConfig::VariablesMap().count("version")) {
    std::cout << "PhASAR " << PhasarConfig::PhasarVersion() << "\n";
    return 0;
  }
  // Vanity header
  if (!PhasarConfig::VariablesMap().count("silent")) {
    std::cout << "PhASAR " << PhasarConfig::PhasarVersion()
              << "\nA LLVM-based static analysis framework\n\n";
  }
  // check if we have anything at all or a call for help
  if (PhasarConfig::VariablesMap().count("help") &&
      !PhasarConfig::VariablesMap().count("silent")) {
    std::cout << Visible << '\n';
    if (PhasarConfig::VariablesMap().count("more-help")) {
      std::cout << MoreHelp << "\n";
    }
    return 0;
  }
  if (!PhasarConfig::VariablesMap().count("silent")) {
    // Print current configuration
    if (PhasarConfig::VariablesMap().count("more-help")) {
      std::cout << Visible << '\n';
      std::cout << MoreHelp << '\n';
      return 0;
    }
  }
  // setup the analysis controller which executes the chosen analyses
  AnalysisStrategy Strategy = AnalysisStrategy::None;
  if (PhasarConfig::VariablesMap().count("analysis-strategy")) {
    Strategy = to_AnalysisStrategy(
        PhasarConfig::VariablesMap()["analysis-strategy"].as<std::string>());
    if (Strategy == AnalysisStrategy::None) {
      std::cout << "Invalid analysis strategy!\n";
      return 0;
    }
  } else {
    Strategy = AnalysisStrategy::WholeProgram;
  }
  if (!PhasarConfig::VariablesMap().count("module")) {
    std::cout << "At least on LLVM target module is required!\n"
                 "Specify a LLVM target module or re-run with '--help'\n";
    return 0;
  }
  ProjectIRDB IRDB(
      PhasarConfig::VariablesMap()["module"].as<std::vector<std::string>>(),
      IRDBOptions::NONE);
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;
  if (PhasarConfig::VariablesMap().count("data-flow-analysis")) {
    auto Analyses = PhasarConfig::VariablesMap()["data-flow-analysis"]
                        .as<std::vector<std::string>>();
    for (auto &Analysis : Analyses) {
      DataFlowAnalyses.push_back(to_DataFlowAnalysisType(Analysis));
    }
  } else {
    DataFlowAnalyses.push_back(DataFlowAnalysisType::None);
  }
  std::vector<std::string> AnalysisConfigs;
  if (PhasarConfig::VariablesMap().count("analysis-config")) {
    AnalysisConfigs = PhasarConfig::VariablesMap()["analysis-config"]
                          .as<std::vector<std::string>>();
  }
  std::set<std::string> EntryPoints;
  if (PhasarConfig::VariablesMap().count("entry-points")) {
    auto Entries = vectorToSet(PhasarConfig::VariablesMap()["entry-points"]
                                   .as<std::vector<std::string>>());
    EntryPoints.insert(Entries.begin(), Entries.end());
  } else {
    EntryPoints.insert("main");
  }
  AnalysisController Controller(IRDB, DataFlowAnalyses, AnalysisConfigs,
                                EntryPoints, Strategy);
  return 0;
}
