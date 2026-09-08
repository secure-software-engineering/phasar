/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/AnalysisStrategy/Strategies.h"
#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/CallGraphData.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/ExternCallbackModel.h"
#include "phasar/PhasarLLVM/ControlFlow/GlobalCtorsDtorsModel.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSetData.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/InitPhasar.h"
#include "phasar/Utils/Lazy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/WithColor.h"

#include "Controller/AnalysisController.h"
#include "Controller/AnalysisControllerEmitterOptions.h"

#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

using namespace psr;

namespace cl = llvm::cl;

namespace {

cl::OptionCategory PsrCat("PhASAR");

static auto values(std::initializer_list<cl::OptionEnumValue> IList) {
  return cl::ValuesClass(IList);
}

#define PSR_OPTION_FLAG(NAME, CMDFLAG, DESC, ...)                              \
  cl::opt<bool> NAME(CMDFLAG, cl::desc(DESC), cl::cat(PsrCat), ##__VA_ARGS__)

#define PSR_SHORTLONG_OPTION_TYPE(NAME, TYPE, SHORTCMD, LONGCMD, DESC, ...)    \
  TYPE NAME(LONGCMD, cl::desc(DESC), cl::cat(PsrCat), ##__VA_ARGS__);          \
  cl::alias NAME##ShortAlias(SHORTCMD, cl::aliasopt(NAME),                     \
                             cl::desc(DESC " (alias for --" LONGCMD ")"),      \
                             cl::cat(PsrCat))

#define PSR_SHORTLONG_OPTION(NAME, TYPE, SHORTCMD, LONGCMD, DESC, ...)         \
  PSR_SHORTLONG_OPTION_TYPE(NAME, cl::opt<TYPE>, SHORTCMD, LONGCMD, DESC,      \
                            ##__VA_ARGS__)

// PSR_SHORTLONG_OPTION(ConfigOpt, std::string, "c", "config",
//                      "Path to the configuration file, options can be "
//                      "specified as 'parameter = option'");

PSR_SHORTLONG_OPTION(SilentOpt, bool, "s", "silent",
                     "Suppress any non-result output");
cl::alias QuietAlias("quiet", cl::aliasopt(SilentOpt),
                     cl::desc("Alias for --silent"), cl::cat(PsrCat));

PSR_SHORTLONG_OPTION(ModuleOpt, std::string, "m", "module",
                     "Path to the LLVM IR module under analysis", cl::Required);

PSR_SHORTLONG_OPTION_TYPE(
    EntryOpt, cl::list<std::string>, "E", "entry-points",
    "Set the entry point(s) to be used; use '__ALL__' to specify all available "
    "function definitions as entry points");

cl::list<DataFlowAnalysisType>
    DataFlowAnalysisOpt("data-flow-analysis",
                        cl::desc("Set the analyses to be run"),
                        values({
#define DATA_FLOW_ANALYSIS_TYPES(NAME, CMDFLAG, DESC)                          \
  clEnumValN(DataFlowAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.def"
                        }),
                        cl::cat(PsrCat));
cl::alias DataFlowAnalysisAlias("D", cl::aliasopt(DataFlowAnalysisOpt),
                                cl::desc("Alias for --data-flow-analysis"),
                                cl::cat(PsrCat));

cl::opt<AnalysisStrategy> StrategyOpt("analysis-strategy",
                                      cl::desc("The analysis strategy"),
                                      values({
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, DESC)                           \
  clEnumValN(AnalysisStrategy::NAME, CMDFLAG, DESC),
#include "phasar/AnalysisStrategy/Strategies.def"
                                      }),
                                      cl::init(AnalysisStrategy::WholeProgram),
                                      cl::cat(PsrCat), cl::Hidden);

cl::opt<std::string> AnalysisConfigOpt(
    "analysis-config",
    cl::desc("Set the analysis's configuration (if required)"),
    cl::cat(PsrCat));

cl::opt<AliasAnalysisType> AliasTypeOpt(
    "alias-analysis",
    cl::desc("Set the alias analysis to be used (CFLSteens, "
             "CFLAnders).  CFLSteens is ~O(N) but inaccurate while "
             "CFLAnders O(N^3) but more accurate."),
    values({
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                               \
  clEnumValN(AliasAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/Pointer/AliasAnalysisType.def"
    }),
    cl::init(AliasAnalysisType::CFLAnders), cl::cat(PsrCat));
cl::alias AliasTypeAlias("P", cl::aliasopt(AliasTypeOpt),
                         cl::desc("Alias for --alias-analysis"),
                         cl::cat(PsrCat));
cl::opt<UnionFindAliasAnalysisType> UFAliasTypeOpt(
    "union-find-aa",
    cl::desc(
        "The union-find alias analysis type to use (default: ctx-ind-sens)"),
    cl::init(UnionFindAliasAnalysisType::CtxIndSens), cl::cat(PsrCat),
    values({
#define UNIONFIND_ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                     \
  clEnumValN(UnionFindAliasAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/Pointer/UnionFindAAType.def"
    }));

cl::opt<CallGraphAnalysisType>
    CGTypeOpt("call-graph-analysis",
              cl::desc("Set the call-graph algorithm to be used"),
              values({
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  clEnumValN(CallGraphAnalysisType::NAME, CMDFLAG, DESC),
#include "phasar/ControlFlow/CallGraphAnalysisType.def"
              }),
              cl::init(CallGraphAnalysisType::OTF), cl::cat(PsrCat));
cl::alias CGTypeAlias("C", cl::aliasopt(CGTypeOpt),
                      cl::desc("Alias for --call-graph-analysis"),
                      cl::cat(PsrCat));

cl::opt<Soundness> SoundnessOpt("soundness",
                                cl::desc("Set the soundiness level to be used"),
                                values({
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, DESC)                               \
  clEnumValN(Soundness::NAME, CMDFLAG, DESC),
#include "phasar/Utils/Soundness.def"
                                }),
                                cl::init(Soundness::Soundy), cl::cat(PsrCat),
                                cl::Hidden);
PSR_OPTION_FLAG(AutoGlobalsOpt, "auto-globals",
                "Enable automated support for global initializers",
                cl::init(true));

PSR_OPTION_FLAG(ExternalCallsRewriteOpt, "rewrite-external-calls",
                "Whether to rewrite calls-to known external functions, such as "
                "pthread_create, s.t., their callback calls are not lost in "
                "the call-graph",
                cl::init(true));

PSR_SHORTLONG_OPTION(
    StatisticsOpt, bool, "S", "emit-stats",
    "Collect and emit statistics of the module(s) under analysis");

#ifdef DYNAMIC_LOG
PSR_SHORTLONG_OPTION(LogOpt, bool, "L", "log", "Enable logging");
cl::opt<SeverityLevel> LogSeverityOpt(
    "log-level",
    cl::desc(
        "The minimum severity-level a log message must have in order to be "
        "printed. Has no effect if logging is disabled. You can enable "
        "logging with --log/-L or by specifying at least one --log-cat."),
    cl::cat(PsrCat),
    values({
#define SEVERITY_LEVEL(NAME, TYPE) clEnumValN(SeverityLevel::TYPE, NAME, NAME),
#include "phasar/Utils/SeverityLevel.def"
    }),
    cl::init(SeverityLevel::DEBUG));

cl::list<std::string>
    LogCategoriesOpt("log-cat",
                     cl::desc("The categories that should be enabled for "
                              "logging. Implies --log/-L is non-empty."),
                     cl::cat(PsrCat));
#endif

cl::opt<std::string>
    ExportOpt("export",
              cl::desc("Export mode (JSON, SARIF) (Not implemented yet!)"),
              cl::cat(PsrCat), cl::Hidden);

cl::opt<std::string> ProjectIdOpt("project-id",
                                  cl::desc("Project id used for output"),
                                  cl::cat(PsrCat), cl::Hidden);

PSR_SHORTLONG_OPTION(OutDirOpt, std::string, "O", "out",
                     "Output directory; if specified all results are written "
                     "to the output directory instead of stdout");

PSR_OPTION_FLAG(EmitIROpt, "emit-ir",
                "Emit preprocessed and annotated IR of analysis target");
PSR_OPTION_FLAG(EmitRawResultsOpt, "emit-raw-results",
                "Emit unprocessed/raw solver results");
PSR_OPTION_FLAG(EmitTextReportOpt, "emit-text-report",
                "Emit textual report of solver results", cl::init(true));
PSR_OPTION_FLAG(EmitGraphicalReportOpt, "emit-graphical-report",
                "Emit graphical report of solver results", cl::Hidden);
PSR_OPTION_FLAG(EmitESGAsDotOpt, "emit-esg-as-dot",
                "Emit the exploded super-graph (ESG) as DOT graph");
PSR_OPTION_FLAG(EmitTHAsTextOpt, "emit-th-as-text",
                "Emit the type hierarchy as text");
PSR_OPTION_FLAG(EmitTHAsDotOpt, "emit-th-as-dot",
                "Emit the type hierarchy as DOT graph");
PSR_OPTION_FLAG(EmitTHAsJsonOpt, "emit-th-as-json",
                "Emit the type hierarchy as JSON");
PSR_OPTION_FLAG(EmitCGAsTextOpt, "emit-cg-as-text",
                "Emit the call graph as text");
PSR_OPTION_FLAG(EmitCGAsDotOpt, "emit-cg-as-dot",
                "Emit the call graph as DOT graph");
PSR_OPTION_FLAG(EmitCGAsJsonOpt, "emit-cg-as-json",
                "Emit the call graph as json");
PSR_OPTION_FLAG(EmitPTAAsTextOpt, "emit-pta-as-text",
                "Emit the points-to information as text");
PSR_OPTION_FLAG(EmitPTAAsDotOpt, "emit-pta-as-dot",
                "Emit the points-to information as DOT graph");
PSR_OPTION_FLAG(EmitPTAAsJsonOpt, "emit-pta-as-json",
                "Emit the points-to information as json");
PSR_OPTION_FLAG(EmitStatsAsJsonOpt, "emit-statistics-as-json",
                "Emit the statistics information as json");
PSR_OPTION_FLAG(FollowReturnPastSeedsOpt, "follow-return-past-seeds",
                "Let the IFDS/IDE Solver process unbalanced returns",
                cl::init(true));
PSR_OPTION_FLAG(AutoAddZeroOpt, "auto-add-zero",
                "Let the IFDS/IDE Solver automatically add the special zero "
                "value to any set of dataflow-facts",
                cl::init(true));
PSR_OPTION_FLAG(
    ComputeValuesOpt, "compute-values",
    "Let the IDE Solver compute the values attached to each edge in the ESG",
    cl::init(true));
PSR_OPTION_FLAG(
    RecordEdgesOpt, "record-edges",
    "Let the IFDS/IDE Solver record all ESG edges whole solving the dataflow "
    "problem. This can have massive performance impact",
    cl::Hidden);
PSR_OPTION_FLAG(PersistedSummariesOpt, "persisted-summaries",
                "Let the IFDS/IDE Solver compute persisted procedure summaries "
                "(Currently not supported)",
                cl::Hidden);

cl::opt<std::string>
    LoadPTAFromJsonOpt("load-pta-from-json",
                       cl::desc("Load the points-to info previously exported "
                                "via emit-pta-as-json from the given file"),
                       cl::cat(PsrCat));

cl::opt<std::string> LoadCGFromJsonOpt(
    "load-cg-from-json",
    cl::desc("Load the persisted call-graph previously exported via "
             "emit-cg-as-json from the given file"),
    cl::cat(PsrCat));

PSR_SHORTLONG_OPTION(PammOutOpt, std::string, "A", "pamm-out",
                     "Filename for PAMM's gathered data",
                     cl::init("PAMM_data.json"), cl::cat(PsrCat), cl::Hidden);

void validateParamModule() {
  if (!(llvm::sys::fs::exists(ModuleOpt) &&
        !llvm::sys::fs::is_directory(ModuleOpt) &&
        (llvm::is_contained(llvm::ArrayRef{".bc", ".ll"},
                            llvm::sys::path::extension(ModuleOpt))))) {
    llvm::SmallString<256> RealModPath;
    auto EC = llvm::sys::fs::real_path(ModuleOpt, RealModPath);
    llvm::WithColor::error()
        << "LLVM module '" << (EC ? ModuleOpt.getValue() : RealModPath.str())
        << "' does not exist!\n";
    exit(1);
  }
}

void validateParamOutput() {
  if (!OutDirOpt.empty() &&
      !llvm::sys::fs::is_directory(OutDirOpt.getValue())) {
    llvm::WithColor::error()
        << '\'' << OutDirOpt
        << "' does not exist, a valid output directory is required!\n";
    exit(1);
  }
}

void validateParamAnalysisConfig() {
  if (!AnalysisConfigOpt.empty() &&
      !(llvm::sys::fs::exists(AnalysisConfigOpt.getValue()) &&
        !llvm::sys::fs::is_directory(AnalysisConfigOpt.getValue()))) {
    llvm::WithColor::error() << "Analysis configuration '" << AnalysisConfigOpt
                             << "' does not exist!\n";
    exit(1);
  }
}

void validatePTAJsonFile() {
  if (!LoadPTAFromJsonOpt.empty() &&
      !(llvm::sys::fs::exists(LoadPTAFromJsonOpt.getValue()) &&
        !llvm::sys::fs::is_directory(LoadPTAFromJsonOpt.getValue()))) {
    llvm::WithColor::error() << "Points-to info file '" << LoadPTAFromJsonOpt
                             << "' does not exist!\n";
    exit(1);
  }
}

std::vector<std::string> setupIRAndEntrypoints(LLVMProjectIRDB &IRDB) {
  std::vector<std::string> EntryPoints = std::move(EntryOpt);
  if (EntryPoints.empty()) {
    EntryPoints = getDefaultEntryPoints(IRDB);
  }
  if (AutoGlobalsOpt) {
    GlobalCtorsDtorsModel::buildModel(IRDB, EntryPoints);
    EntryPoints = {GlobalCtorsDtorsModel::ModelName.str()};
  }
  if (ExternalCallsRewriteOpt) {
    ExternCallbackModel::rewriteCalls(IRDB);
  }
  return EntryPoints;
}

[[nodiscard]] AnalysisControllerEmitterOptions setupEmitterOptions() {
  auto EmitterOptions = AnalysisControllerEmitterOptions::None;

  if (EmitIROpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitIR;
  }
  if (EmitRawResultsOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitRawResults;
  }
  if (EmitTextReportOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTextReport;
  }
  if (EmitGraphicalReportOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitGraphicalReport;
  }
  if (EmitESGAsDotOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitESGAsDot;
  }
  if (EmitTHAsTextOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsText;
  }
  if (EmitTHAsDotOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsDot;
  }
  if (EmitTHAsJsonOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitTHAsJson;
  }
  if (EmitCGAsDotOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsDot;
  }
  if (EmitCGAsJsonOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitCGAsJson;
  }
  if (EmitCGAsTextOpt) {
    llvm::WithColor::error()
        << "'--emit-cg-as-text' is currently not supported. Did you mean "
           "'--emit-cg-as-dot'? For reversible serialization use "
           "'--emit-cg-as-json'\n";
    exit(1);
  }
  if (EmitPTAAsTextOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsText;
  }
  if (EmitPTAAsDotOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsDot;
  }
  if (EmitPTAAsJsonOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitPTAAsJson;
  }
  if (StatisticsOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitStatisticsAsText;
  }
  if (EmitStatsAsJsonOpt) {
    EmitterOptions |= AnalysisControllerEmitterOptions::EmitStatisticsAsJson;
  }
  return EmitterOptions;
}

[[nodiscard]] IFDSIDESolverConfig setupSolverConfig() {
  IFDSIDESolverConfig SolverConfig{};
  SolverConfig.setFollowReturnsPastSeeds(FollowReturnPastSeedsOpt);
  SolverConfig.setAutoAddZero(AutoAddZeroOpt);
  SolverConfig.setComputeValues(ComputeValuesOpt);
  SolverConfig.setRecordEdges(RecordEdgesOpt || EmitESGAsDotOpt);
  SolverConfig.setComputePersistedSummaries(PersistedSummariesOpt);
  SolverConfig.setEmitESG(EmitESGAsDotOpt);
  return SolverConfig;
}

} // anonymous namespace

int main(int Argc, const char **Argv) {
  PSR_INITIALIZER(Argc, Argv);

  cl::SetVersionPrinter([](llvm::raw_ostream &OS) {
    OS << "PhASAR " << PhasarConfig::PhasarVersion() << '\n';
  });
  cl::HideUnrelatedOptions(PsrCat);
  cl::ParseCommandLineOptions(Argc, Argv);

#ifdef DYNAMIC_LOG
  if (LogOpt) {
    Logger::initializeStderrLogger(LogSeverityOpt);
  } else if (!SilentOpt) {
    Logger::initializeStderrLogger(SeverityLevel::ERROR);
  }
  for (const auto &LogCat : LogCategoriesOpt) {
    Logger::initializeStderrLogger(LogSeverityOpt, LogCat);
  }
#endif

  // Vanity header
  if (!SilentOpt) {
    llvm::outs() << "PhASAR " << PhasarConfig::PhasarVersion()
                 << "\nA LLVM-based static analysis framework\n\n";
  }

  validateParamModule();
  validateParamOutput();
  validateParamAnalysisConfig();
  validatePTAJsonFile();

  [[maybe_unused]] auto &PConfig = PhasarConfig::getPhasarConfig();

  // setup the emitter options to display the computed analysis results
  auto EmitterOptions = setupEmitterOptions();

  std::optional<LLVMAliasSetData> PrecomputedAliasSet;
  if (!LoadPTAFromJsonOpt.empty()) {
    PHASAR_LOG_LEVEL(INFO, "Load AliasInfo from file: " << LoadCGFromJsonOpt);
    PrecomputedAliasSet = LLVMAliasSetData::deserializeJson(LoadPTAFromJsonOpt);
  }

  std::optional<CallGraphData> PrecomputedCallGraph;
  if (!LoadCGFromJsonOpt.empty()) {
    PHASAR_LOG_LEVEL(INFO, "Load CallGraph from file: " << LoadCGFromJsonOpt);
    PrecomputedCallGraph = CallGraphData::deserializeJson(LoadCGFromJsonOpt);
  }

  auto IRDB = std::make_unique<LLVMProjectIRDB>(
      PSR_LAZY(LLVMProjectIRDB::loadOrExit(ModuleOpt)));

  auto EntryPoints = setupIRAndEntrypoints(*IRDB);

  // create project id
  llvm::SmallString<128> ProjectId(ProjectIdOpt);
  if (ProjectId.empty()) {
    ProjectId.assign(llvm::sys::path::filename(ModuleOpt));
    llvm::sys::path::replace_extension(ProjectId, {});
    if (ProjectId.empty()) {
      ProjectId = "default-phasar-project";
    }
  }

  // create directory for results
  llvm::SmallString<128> OutDir(OutDirOpt);
  if (!OutDir.empty()) {
    llvm::sys::path::append(OutDir,
                            ProjectId + llvm::Twine("-") + createTimeStamp());
    auto EC = llvm::sys::fs::create_directory(OutDir);
    if (EC) {
      llvm::WithColor::error() << EC.message() << '\n';
      return 1;
    }
  }

  AnalysisController Controller{
      .HA = HelperAnalyses(
          std::move(IRDB), std::move(EntryPoints),
          {
              .PrecomputedPTS = std::move(PrecomputedAliasSet),
              .PrecomputedCG = std::move(PrecomputedCallGraph),
              .PTATy = AliasTypeOpt,
              .UFAATy = UFAliasTypeOpt,
              .CGTy = CGTypeOpt,
              .SoundnessLevel = SoundnessOpt,
              .AutoGlobalSupport =
                  false, // already handled in setupIRAndEntrypoints()
              .AllowLazyPTS =
                  !AnalysisController::needsToEmitPTA(EmitterOptions),
          }),
      .DataFlowAnalyses = DataFlowAnalysisOpt,
      .AnalysisConfigs = {AnalysisConfigOpt.getValue()},
      .Strategy = StrategyOpt,
      .EmitterOptions = EmitterOptions,
      .SolverConfig = setupSolverConfig(),
      .ProjectID = std::move(ProjectId),
      .ResultDirectory = std::move(OutDir),
  };

  Controller.emitRequestedHelperAnalysisResults();
  Controller.run();
  return 0;
}
