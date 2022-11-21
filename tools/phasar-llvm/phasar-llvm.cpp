/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Config/Configuration.h"
#include "phasar/Controller/AnalysisController.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Soundness.h"

#include "boost/program_options.hpp"
#include "boost/program_options/value_semantic.hpp"

#include "llvm/ADT/StringRef.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace psr;

namespace std {

std::ostream &operator<<(std::ostream &OS, const std::vector<std::string> &V) {
  for (const auto &Str : V) {
    OS << Str;
    if (Str != V.back()) {
      OS << ", ";
    }
  }
  return OS;
}

} // namespace std

namespace {

constexpr char MoreHelp[] =
#include "../phasar-llvm_more_help.txt"
    ;

template <typename T> static std::set<T> vectorToSet(const std::vector<T> &V) {
  std::set<T> S;
  for_each(V.begin(), V.end(), [&S](T Item) { S.insert(Item); });
  return S;
}

void validateParamConfigFile(const std::string &Config) {
  if (!(std::filesystem::exists(Config) &&
        !std::filesystem::is_directory(Config))) {
    throw boost::program_options::error_with_option_name(
        "PhASAR configuration '" + Config + "' does not exist!");
  }
}

void validateParamModule(const std::string &Module) {

  std::filesystem::path ModulePath(Module);
  if (!(std::filesystem::exists(ModulePath) &&
        !std::filesystem::is_directory(ModulePath) &&
        (ModulePath.extension() == ".ll" || ModulePath.extension() == ".bc"))) {
    throw boost::program_options::error_with_option_name(
        "LLVM module '" + Module + "' does not exist!");
  }
}

void validateParamExport(const std::string & /*Export*/) {
  throw boost::program_options::error_with_option_name(
      "Parameter not supported, yet.");
}

void validateParamOutput(const std::string &Output) {
  if (!Output.empty() && !std::filesystem::is_directory(Output)) {
    throw boost::program_options::error_with_option_name(
        "'" + Output +
        "' does not exist, a valid output directory is required!");
  }
}

void validateParamPammOutputFile(const std::string &Output) {}

void validateParamDataFlowAnalysis(const std::vector<std::string> &Analyses) {
  for (const auto &Analysis : Analyses) {
    if (toDataFlowAnalysisType(Analysis) == DataFlowAnalysisType::None) {
      // throw boost::program_options::error_with_option_name(
      //    "'" + Analysis + "' is not a valid data-flow analysis!");
      llvm::errs() << "Error: " << '\'' << Analysis
                   << "' is not a valid data-flow analysis!" << '\n';
      exit(1);
    }
  }
}

void validateParamAnalysisStrategy(const std::string &Strategy) {
  if (toAnalysisStrategy(Strategy) == AnalysisStrategy::None) {
    throw boost::program_options::error_with_option_name(
        "Invalid analysis strategy '" + Strategy + "'!");
  }
}

void validateParamPointerAnalysis(const std::string &Analysis) {
  if (toPointerAnalysisType(Analysis) == PointerAnalysisType::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Analysis + "' is not a valid pointer analysis!");
  }
}

void validateParamCallGraphAnalysis(const std::string &Analysis) {
  if (toCallGraphAnalysisType(Analysis) == CallGraphAnalysisType::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Analysis + "' is not a valid call-graph analysis!");
  }
}

void validateSoundnessFlag(const std::string &Flag) {
  if (toSoundness(Flag) == Soundness::Invalid) {
    throw boost::program_options::error_with_option_name(
        "'" + Flag + "' is not a valid soundness level!");
  }
}

void validateParamAnalysisConfig(const std::vector<std::string> &Configs) {
  for (const auto &Config : Configs) {
    if (!(std::filesystem::exists(Config) &&
          !std::filesystem::is_directory(Config))) {
      throw boost::program_options::error_with_option_name(
          "Analysis configuration '" + Config + "' does not exist!");
    }
  }
}

void validatePTAJsonFile(const std::string &Config) {
  if (!(std::filesystem::exists(Config) &&
        !std::filesystem::is_directory(Config))) {
    throw boost::program_options::error_with_option_name(
        "Points-to info file '" + Config + "' does not exist!");
  }
}

void validateLogLevel(const std::string &Level) {
  if (parseSeverityLevel(Level) == SeverityLevel::INVALID) {
    throw boost::program_options::error_with_option_name(
        "Invalid logger severity level '" + Level +
        "'. Expect DEBUG, INFO, WARNING or ERROR");
  }
}

} // anonymous namespace

int main(int Argc, const char **Argv) {
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
			("module,m", boost::program_options::value<std::string>()->notifier(&validateParamModule), "Path to the module(s) under analysis")
      ("entry-points,E", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->default_value(std::vector({std::string("main")})), "Set the entry point(s) to be used; use '__ALL__' to specify all available function definitions as entry points")
			("data-flow-analysis,D", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()/*->notifier(&validateParamDataFlowAnalysis)*/, "Set the analysis to be run")
			("analysis-strategy", boost::program_options::value<std::string>()->default_value("WPA")->notifier(&validateParamAnalysisStrategy))
      ("analysis-config", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing()->notifier(&validateParamAnalysisConfig), "Set the analysis's configuration (if required)")
      ("pointer-analysis,P", boost::program_options::value<std::string>()->notifier(&validateParamPointerAnalysis)->default_value("CFLAnders"), "Set the points-to analysis to be used (CFLSteens, CFLAnders).  CFLSteens is ~O(N) but inaccurate while CFLAnders O(N^3) but more accurate.")
      ("call-graph-analysis,C", boost::program_options::value<std::string>()->notifier(&validateParamCallGraphAnalysis)->default_value("OTF"), "Set the call-graph algorithm to be used (NORESOLVE, CHA, RTA, DTA, VTA, OTF)")
      ("soundness", boost::program_options::value<std::string>()->notifier(&validateSoundnessFlag)->default_value("Soundy"), "Set the soundiness level to be used (Sound, Soundy, Unsound)")
      ("auto-globals", boost::program_options::value<bool>()->default_value(true), "Enable automated global support")
      ("classhierarchy-analysis,H", "Class-hierarchy analysis")
			("statistical-analysis,S", "Statistics")
			("mwa,M", "Enable Modulewise-program analysis mode")
			("printedgerec,R", "Print exploded-super-graph edge recorder")
      #ifdef DYNAMIC_LOG
      ("log,L",boost::program_options::value<std::string>()->notifier(&validateLogLevel), "Enable logging with the specified severity")
      ("log-cat", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(), "Enable logging for the specified categories with the severity given with the -L/--log option. It is invalid to specify --log-cat without specifying -L/--log.")
      #endif
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
      ("emit-th-as-json", "Emit the type hierarchy as JSON")
      // ("emit-cg-as-text", "Emit the call graph as text")
      ("emit-cg-as-dot", "Emit the call graph as DOT graph")
      // ("emit-cg-as-json", "Emit the call graph as JSON")
      ("emit-pta-as-text", "Emit the points-to information as text")
      ("emit-pta-as-dot", "Emit the points-to information as DOT graph")
      ("emit-pta-as-json", "Emit the points-to information as JSON")
      ("emit-statistic-as-json", "Emit the statistic information as JSON")
      ("follow-return-past-seeds", boost::program_options::value<bool>()->default_value(false), "Let the IFDS/IDE Solver process unbalanced returns")
      ("auto-add-zero", boost::program_options::value<bool>()->default_value(true), "Let the IFDS/IDE Solver automatically add the special zero value to any set of dataflow-facts")
      ("compute-values", boost::program_options::value<bool>()->default_value(true), "Let the IDE Solver compute the values attached to each edge in the ESG")
      ("record-edges", boost::program_options::value<bool>()->default_value(true), "Let the IFDS/IDE Solver record all ESG edges whole solving the dataflow problem. This can have massive performance impact")
      ("persisted-summaries", boost::program_options::value<bool>()->default_value(false), "Let the IFDS/IDE Solver compute presisted procedure summaries (Currently not supported)")
      ("load-pta-from-json", boost::program_options::value<std::string>()->notifier(&validatePTAJsonFile),"Load the points-to info previously exported via emit-pta-as-json from the given file")
      ("pamm-out,A", boost::program_options::value<std::string>()->notifier(validateParamPammOutputFile)->default_value("PAMM_data.json"), "Filename for PAMM's gathered data")
      ("right-to-ludicrous-speed", "Uses ludicrous speed (shared memory parallelism) whenever possible (Currently not available)");
  // clang-format on
  boost::program_options::options_description CmdlineOptions;
  CmdlineOptions.add(Generic).add(Config);
  boost::program_options::options_description ConfigFileOptions;
  ConfigFileOptions.add(Config);
  boost::program_options::options_description Visible("Allowed options");
  Visible.add(Generic).add(Config);
  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(Argc, Argv)
            .options(CmdlineOptions)
            .run(),
        PhasarConfig::VariablesMap());
    boost::program_options::notify(PhasarConfig::VariablesMap());
  } catch (boost::program_options::error &Err) {
    llvm::errs() << "Could not parse command-line arguments!\n"
                 << "Error: " << Err.what() << '\n';
    return 1;
  }
  try {
    if (!ConfigFile.empty()) {
      std::ifstream Ifs(ConfigFile.c_str());
      if (!Ifs) {
      } else {
        boost::program_options::store(
            boost::program_options::parse_config_file(Ifs, ConfigFileOptions),
            PhasarConfig::VariablesMap());
        boost::program_options::notify(PhasarConfig::VariablesMap());
      }
    }
  } catch (boost::program_options::error &Err) {
    llvm::errs() << "Could not parse configuration file!\n"
                 << "Error: " << Err.what() << '\n';
    return 1;
  }
#ifdef DYNAMIC_LOG
  if (auto LogIt = PhasarConfig::VariablesMap().find("log");
      LogIt != PhasarConfig::VariablesMap().end() &&
      !LogIt->second.as<std::string>().empty()) {
    auto Sev = parseSeverityLevel(LogIt->second.as<std::string>());

    Logger::initializeStderrLogger(Sev);

    if (auto LogCatIt = PhasarConfig::VariablesMap().find("log-cat");
        LogCatIt != PhasarConfig::VariablesMap().end()) {
      for (const auto &Cat : LogCatIt->second.as<std::vector<std::string>>()) {
        Logger::initializeStderrLogger(Sev, Cat);
      }
    }
  } else if (PhasarConfig::VariablesMap().count("log-cat")) {
    llvm::errs()
        << "ERROR: The option --log-cat requires -L/--log to be set as well!\n";
    return 1;
  }
#endif
  // print PhASAR version
  if (PhasarConfig::VariablesMap().count("version")) {
    llvm::outs() << "PhASAR " << PhasarConfig::PhasarVersion() << '\n';
    return 0;
  }
  // Vanity header
  if (!PhasarConfig::VariablesMap().count("silent")) {
    llvm::outs() << "PhASAR " << PhasarConfig::PhasarVersion()
                 << "\nA LLVM-based static analysis framework\n\n";
  }
  // check if we have anything at all or a call for help
  if (PhasarConfig::VariablesMap().count("help") &&
      !PhasarConfig::VariablesMap().count("silent")) {
    std::stringstream S;
    S << Visible << '\n';
    if (PhasarConfig::VariablesMap().count("more-help")) {
      S << MoreHelp << '\n';
    }
    llvm::outs() << S.str();
    return 0;
  }
  if (!PhasarConfig::VariablesMap().count("silent")) {
    // Print current configuration
    if (PhasarConfig::VariablesMap().count("more-help")) {
      std::stringstream S;
      S << Visible << '\n';
      S << MoreHelp << '\n';
      llvm::outs() << S.str();
      return 0;
    }
  }
  // setup the analysis controller which executes the chosen analyses
  AnalysisStrategy Strategy = AnalysisStrategy::None;
  if (PhasarConfig::VariablesMap().count("analysis-strategy")) {
    Strategy = toAnalysisStrategy(
        PhasarConfig::VariablesMap()["analysis-strategy"].as<std::string>());
    if (Strategy == AnalysisStrategy::None) {
      llvm::outs() << "Invalid analysis strategy!\n";
      return 0;
    }
  } else {
    Strategy = AnalysisStrategy::WholeProgram;
  }
  if (!PhasarConfig::VariablesMap().count("module")) {
    llvm::outs() << "At least on LLVM target module is required!\n"
                    "Specify a LLVM target module or re-run with '--help'\n";
    return 0;
  }

  // setup phasar project id
  std::string ProjectID = "default-phasar-project";
  if (PhasarConfig::VariablesMap().count("project-id")) {
    ProjectID = PhasarConfig::VariablesMap()["project-id"].as<std::string>();
  }

  // setup output directory
  std::filesystem::path ResultDirectory{};
  if (PhasarConfig::VariablesMap().count("out")) {
    ResultDirectory = PhasarConfig::VariablesMap()["out"].as<std::string>();
    if (!ResultDirectory.empty()) {
      // create directory for results
      ResultDirectory /= ProjectID + "-" + createTimeStamp();
      std::filesystem::create_directory(ResultDirectory);
    }
  }

  bool EmitStats = PhasarConfig::VariablesMap().count("statistical-analysis");
  bool EmitStatsJson =
      PhasarConfig::VariablesMap().count("emit-statistic-as-json");

  // setup IRDB as source code manager
  LLVMProjectIRDB IRDB(
      PhasarConfig::VariablesMap()["module"].as<std::string>());

  if (EmitStats || EmitStatsJson) {
    GeneralStatisticsAnalysis GSA;
    auto Stats = GSA.runOnModule(*IRDB.getModule());

    if (EmitStats) {
      llvm::outs() << "Module " << IRDB.getModule()->getName().str() << ":\n";
      llvm::outs() << "> LLVM IR instructions:\t" << IRDB.getNumInstructions()
                   << '\n';

      llvm::outs() << "> Functions:\t\t" << Stats.getFunctions() << '\n';
      llvm::outs() << "> Global variables:\t" << Stats.getGlobals() << '\n';
      llvm::outs() << "> Alloca instructions:\t"
                   << Stats.getAllocaInstructions().size() << '\n';
      llvm::outs() << "> Call Sites:\t\t" << Stats.getFunctioncalls() << '\n';
    }

    if (EmitStatsJson) {
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream("/psr-IrStatistic.json")) {
          *OFS << Stats.getAsJson().dump(4) << '\n';
        }
      } else {
        llvm::outs() << Stats.getAsJson().dump(4) << '\n';
      }
    }
  }

  // store enabled data-flow analyses
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;

  // setup data-flow analyses
  if (PhasarConfig::VariablesMap().count("data-flow-analysis")) {
    auto Analyses = PhasarConfig::VariablesMap()["data-flow-analysis"]
                        .as<std::vector<std::string>>();
    validateParamDataFlowAnalysis(Analyses);
    for (auto &Analysis : Analyses) {
      DataFlowAnalyses.emplace_back(toDataFlowAnalysisType(Analysis));
    }
  } else {
    DataFlowAnalyses.emplace_back(DataFlowAnalysisType::None);
  }
  // setup the data-flow analyses's corresponding analysis configs if anay
  std::vector<std::string> AnalysisConfigs;
  if (PhasarConfig::VariablesMap().count("analysis-config")) {
    AnalysisConfigs = PhasarConfig::VariablesMap()["analysis-config"]
                          .as<std::vector<std::string>>();
  }
  auto EntryPoints = PhasarConfig::VariablesMap()["entry-points"]
                         .as<std::vector<std::string>>();
  // setup pointer algorithm to be used
  PointerAnalysisType PTATy = toPointerAnalysisType(
      PhasarConfig::VariablesMap()["pointer-analysis"].as<std::string>());
  // setup call-graph algorithm to be used
  CallGraphAnalysisType CGTy = toCallGraphAnalysisType(
      PhasarConfig::VariablesMap()["call-graph-analysis"].as<std::string>());
  // setup soudiness level to be used
  Soundness SoundnessLevel =
      toSoundness(PhasarConfig::VariablesMap()["soundness"].as<std::string>());
  // setup the emitter options to display the computed analysis results
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::EmitTextReport;

  IFDSIDESolverConfig SolverConfig;
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
    SolverConfig.setEmitESG();
  }
  if (PhasarConfig::VariablesMap().count("emit-th-as-text")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsText;
  }
  if (PhasarConfig::VariablesMap().count("emit-th-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsDot;
  }
  if (PhasarConfig::VariablesMap().count("emit-th-as-json")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsJson;
  }
  // if (PhasarConfig::VariablesMap().count("emit-cg-as-text")) {
  //   EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsText;
  // }
  if (PhasarConfig::VariablesMap().count("emit-cg-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsDot;
  }
  // if (PhasarConfig::VariablesMap().count("emit-cg-as-json")) {
  //   EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsJson;
  // }
  if (PhasarConfig::VariablesMap().count("emit-pta-as-text")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsText;
  }
  if (PhasarConfig::VariablesMap().count("emit-pta-as-dot")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsDot;
  }
  if (PhasarConfig::VariablesMap().count("emit-pta-as-json")) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsJson;
  }
  if (PhasarConfig::VariablesMap().count("follow-return-past-seeds")) {
    SolverConfig.setFollowReturnsPastSeeds(
        PhasarConfig::VariablesMap()["follow-return-past-seeds"].as<bool>());
  }
  if (PhasarConfig::VariablesMap().count("auto-add-zero")) {
    SolverConfig.setAutoAddZero(
        PhasarConfig::VariablesMap()["auto-add-zero"].as<bool>());
  }
  if (PhasarConfig::VariablesMap().count("compute-values")) {
    SolverConfig.setComputeValues(
        PhasarConfig::VariablesMap()["compute-values"].as<bool>());
  }
  if (PhasarConfig::VariablesMap().count("record-edges")) {
    SolverConfig.setRecordEdges(
        PhasarConfig::VariablesMap()["record-edges"].as<bool>());
  }
  if (PhasarConfig::VariablesMap().count("persisted-summaries")) {
    SolverConfig.setComputePersistedSummaries(
        PhasarConfig::VariablesMap()["persisted-summaries"].as<bool>());
  }
  nlohmann::json PrecomputedPointsToSet;
  if (auto PTAFile = PhasarConfig::VariablesMap().find("load-pta-from-json");
      PTAFile != PhasarConfig::VariablesMap().end()) {
    PrecomputedPointsToSet =
        readJsonFile(llvm::StringRef(PTAFile->second.as<std::string>()));
  }

  AnalysisController Controller(
      IRDB, std::move(DataFlowAnalyses), std::move(AnalysisConfigs), PTATy,
      CGTy, SoundnessLevel,
      PhasarConfig::VariablesMap()["auto-globals"].as<bool>(), EntryPoints,
      Strategy, EmitterOptions, SolverConfig, ProjectID, ResultDirectory,
      PrecomputedPointsToSet);
  return 0;
}
