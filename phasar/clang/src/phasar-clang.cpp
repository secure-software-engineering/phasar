/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <filesystem>

// #include "phasar/Controller/AnalysisExecutor.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

int main(int Argc, const char **Argv) {
  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "usage: <prog> <ir file>\n";
    return 1;
  }
  Logger::initializeStderrLogger(DEBUG);
  ProjectIRDB DB({Argv[1]}, IRDBOptions::WPA);
  if (DB.getFunction("main")) {
  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}

// else {
//     // -- Clang mode ---
//     std::string ConfigFile;
//     // Declare a group of options that will be allowed only on command line
//     bpo::options_description Generic("Command-line options");
//     // clang-format off
//     Generic.add_options()
//     	("help,h", "Print help message")
//       ("config",
//       bpo::value<std::string>(&ConfigFile)->notifier(validateParamConfig),
//       "Path to the configuration file, options can be specified as 'parameter
//       = option'");
//     // clang-format on
//     // Declare a group of options that will be allowed both on command line
//     and
//     // in config file
//     bpo::options_description Config("Configuration file options");
//     // clang-format off
//     Config.add_options()
//     	("project,p", bpo::value<std::string>()->notifier(validateParamProject),
//     "Path to the project under analysis");
//     // clang-format on
//     bpo::options_description CmdlineOptions;
//     CmdlineOptions.add(PhasarMode).add(Generic).add(Config);
//     bpo::options_description ConfigFileOptions;
//     ConfigFileOptions.add(Config);
//     bpo::options_description Visible("Allowed options");
//     Visible.add(Generic).add(Config);
//     bpo::store(
//         bpo::command_line_parser(argc, argv).options(CmdlineOptions).run(),
//         PhasarConfig::VariablesMap());
//     bpo::notify(PhasarConfig::VariablesMap());
//     ifstream ifs(ConfigFile.c_str());
//     if (!ifs) {
//       LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
//                     << "No configuration file is used.");
//     } else {
//       LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
//                     << "Using configuration file: " << ConfigFile);
//       bpo::store(bpo::parse_config_file(ifs, ConfigFileOptions),
//                  PhasarConfig::VariablesMap());
//       bpo::notify(PhasarConfig::VariablesMap());
//     }
//     // check if we have anything at all or a call for help
//     if (argc < 3 || PhasarConfig::VariablesMap().count("help")) {
//       std::cout << Visible << '\n';
//       return 0;
//     }
//     // Print what has been parsed
//     if (PhasarConfig::VariablesMap().count("project")) {
//       std::cout << "Project: "
//                 << PhasarConfig::VariablesMap()["project"].as<std::string>()
//                 << '\n';
//     }
//     // Bring Clang source-to-source transformation to life
//     if (PhasarConfig::VariablesMap().count("project")) {
//       int OnlyTakeCareOfSources = 2;
//       const char *ProjectSources =
//           PhasarConfig::VariablesMap()["project"].as<std::string>().c_str();
//       const char *DummyProgName = "not_important";
//       const char *DummyArgs[] = {DummyProgName, ProjectSources};
//       clang::tooling::CommonOptionsParser OptionsParser(
//           OnlyTakeCareOfSources, DummyArgs, StaticAnalysisCategory,
//           OccurrencesFlag);
//       // At this point we have set-up all the parameters and can start the
//       // actual
//       // analyses that have been choosen.
//       ClangController CC(OptionsParser);
//     }
//   }
//   LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
//                 << "Shutdown llvm and the analysis framework.");
//   // free all resources handled by llvm
//   llvm::llvm_shutdown();
//   // flush the log core at last (performs flush() on all registered sinks)
//   boost::log::core::get()->flush();
//   STOP_TIMER("Phasar Runtime", PAMM_SEVERITY_LEVEL::Core);
//   // PRINT_MEASURED_DATA(std::cout);
//   EXPORT_MEASURED_DATA(PhasarConfig::VariablesMap()["pamm-out"].as<std::string>());
