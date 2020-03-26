/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <chrono>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/SoundnessFlag.h"

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

void validateParamExport(const std::string &Export) {
  throw boost::program_options::error_with_option_name(
      "Parameter not supported, yet.");
}

void validateParamOutput(const std::string &Output) {
  if (Output != "" && !boost::filesystem::is_directory(Output)) {
    throw boost::program_options::error_with_option_name(
        "'" + Output +
        "' does not exist, a valid output directory is required!");
  }
}

void validateParamPammOutputFile(const std::string &Output) {}

void validateParamDataFlowAnalysis(const std::vector<std::string> &Analyses) {
  for (auto &Analysis : Analyses) {
    if (to_DataFlowAnalysisType(Analysis) == DataFlowAnalysisType::None) {
      throw boost::program_options::error_with_option_name(
          "'" + Analysis + "' is not a valid data-flow analysis!");
    }
  }
}

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

void validateSoundnessFlag(const std::string &Flag) {
  if (to_SoundnessFlag(Flag) == SoundnessFlag::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Flag + "' is not a valid soundiness flag!");
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
			("data-flow-analysis,D", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamDataFlowAnalysis), "Set the analysis to be run")
			("analysis-strategy", boost::program_options::value<std::string>()->default_value("WPA")->notifier(&validateParamAnalysisStrategy))
      ("analysis-config", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamAnalysisConfig), "Set the analysis's configuration (if required)")
      ("pointer-analysis,P", boost::program_options::value<std::string>()->notifier(&validateParamPointerAnalysis)->default_value("CFLAnders"), "Set the points-to analysis to be used (CFLSteens, CFLAnders)")
      ("call-graph-analysis,C", boost::program_options::value<std::string>()->notifier(&validateParamCallGraphAnalysis)->default_value("OTF"), "Set the call-graph algorithm to be used (NORESOLVE, CHA, RTA, DTA, VTA, OTF)")
      ("soundiness-flag", boost::program_options::value<std::string>()->notifier(&validateSoundnessFlag)->default_value("SOUNDY"), "Set the soundiness level to be used (SOUND,SOUNDY,UNSOUND)")
			("classhierarchy-analysis,H", "Class-hierarchy analysis")
			("statistical-analysis,S", "Statistics")
			("mwa,M", "Enable Modulewise-program analysis mode")
			("printedgerec,R", "Print exploded-super-graph edge recorder")
      ("log,L", "Enable logging")
      ("export,E", boost::program_options::value<std::string>()->notifier(&validateParamExport), "Export mode (JSON, SARIF) (Not implemented yet!)")
      ("project-id,I", boost::program_options::value<std::string>()->default_value("default-phasar-project"), "Project id used for output")
      ("out,O", boost::program_options::value<std::string>()->notifier(&validateParamOutput)->default_value(""), "Output directory; if specified all results are written to the output directory instead of stdout")
      ("emit-ir", "Emit preprocessed and annotated IR of analysis target")
      ("emit-raw-results", "Emit unprocessed/raw solver results")
      ("emit-text-report", "Emit textual report of solver results")
      ("emit-graphical-report", "Emit graphical report of solver results")
      ("emit-esg-as-dot", "Emit the exploded super-graph (ESG) as DOT graph")
      ("emit-th-as-text", "Emit the type hierarchy as text")
      ("emit-th-as-dot", "Emit the type hierarchy as DOT graph")
      ("emit-cg-as-text", "Emit the call graph as text")
      ("emit-cg-as-dot", "Emit the call graph as DOT graph")
      ("emit-pta-as-text", "Emit the points-to information as text")
      ("emit-pta-as-dot", "Emit the points-to information as DOT graph")
      ("pamm-out,A", boost::program_options::value<std::string>()->notifier(validateParamPammOutputFile)->default_value("PAMM_data.json"), "Filename for PAMM's gathered data")
      #ifdef PHASAR_PLUGINS_ENABLED
			("analysis-plugin", boost::program_options::value<std::vector<std::string>>()->notifier(&validateParamAnalysisPlugin), "Analysis plugin(s) (absolute path to the shared object file(s))")
      ("callgraph-plugin", boost::program_options::value<std::string>()->notifier(&validateParamICFGPlugin), "ICFG plugin (absolute path to the shared object file)")
      #endif
      ("right-to-ludicrous-speed", "Uses ludicrous speed (shared memory parallelism) whenever possible");
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
  // setup IRDB as source code manager
  ProjectIRDB IRDB(
      PhasarConfig::VariablesMap()["module"].as<std::vector<std::string>>());
  // setup data-flow analyses
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
  // setup the data-flow analyses's corresponding analysis configs if anay
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
  // setup pointer algorithm to be used
  PointerAnalysisType PTATy = to_PointerAnalysisType(
      PhasarConfig::VariablesMap()["pointer-analysis"].as<std::string>());
  // setup call-graph algorithm to be used
  CallGraphAnalysisType CGTy = to_CallGraphAnalysisType(
      PhasarConfig::VariablesMap()["call-graph-analysis"].as<std::string>());
  // setup soudiness level to be used
  SoundnessFlag SF = to_SoundnessFlag(
      PhasarConfig::VariablesMap()["soundiness-flag"].as<std::string>());
  // setup the emitter options to display the computed analysis results
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::EmitTextReport;
  if (PhasarConfig::VariablesMap().count("emit-ir")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitIR;
  }
  if (PhasarConfig::VariablesMap().count("emit-raw-results")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitRawResults;
  }
  if (PhasarConfig::VariablesMap().count("emit-text-report")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTextReport;
  }
  if (PhasarConfig::VariablesMap().count("emit-graphical-report")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitGraphicalReport;
  }
  if (PhasarConfig::VariablesMap().count("emit-esg-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitESGAsDot;
  }
  if (PhasarConfig::VariablesMap().count("emit-th-as-text")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsText;
  }
  if (PhasarConfig::VariablesMap().count("emit-th-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsDot;
  }
  if (PhasarConfig::VariablesMap().count("emit-cg-as-text")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsText;
  }
  if (PhasarConfig::VariablesMap().count("emit-cg-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsDot;
  }
  if (PhasarConfig::VariablesMap().count("emit-pta-as-text")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsText;
  }
  if (PhasarConfig::VariablesMap().count("emit-pta-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsDOT;
  }
  // setup output directory
  std::string OutDirectory;
  if (PhasarConfig::VariablesMap().count("out")) {
    OutDirectory = PhasarConfig::VariablesMap()["out"].as<std::string>();
  }
  // setup phasar project id
  std::string ProjectID;
  if (PhasarConfig::VariablesMap().count("project-id")) {
    ProjectID = PhasarConfig::VariablesMap()["project-id"].as<std::string>();
  }
  AnalysisController Controller(IRDB, DataFlowAnalyses, AnalysisConfigs, PTATy,
                                CGTy,SF, EntryPoints, Strategy, EmitterOptions,
                                ProjectID, OutDirectory);
  return 0;
}
