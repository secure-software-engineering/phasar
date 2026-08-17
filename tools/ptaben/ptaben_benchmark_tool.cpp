#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/TypedArray.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include "PTAResult.h"
#include "PTAUtils.h"
#include "QueryId.h"
#include "QueryLocation.h"
#include "QuerySer.h"
#include "ResultsCollector.h"
#include "SupportedAnalysisTypes.h"

#include <memory>
#include <string>

namespace cl = llvm::cl;

static cl::OptionCategory PTABenCat("PTABen Benchmark Tool");

static cl::SubCommand
    CheckFileCmd("check-file", "Check a single file instead of a directory");

static cl::list<std::string> IRPaths(cl::Positional, cl::OneOrMore,
                                     cl::desc("ptaben-ir-directory"),
                                     cl::cat(PTABenCat),
                                     cl::sub(cl::SubCommand::getTopLevel()));

static cl::opt<std::string> IRFilePath(cl::Positional, cl::Required,
                                       cl::desc("ptaben-ir-file"),
                                       cl::cat(PTABenCat),
                                       cl::sub(CheckFileCmd));
static cl::opt<std::string>
    QueryTablePath("queries-table",
                   cl::desc("The Output-Path to the queries table"),
                   cl::init("queries.csv"), cl::cat(PTABenCat));

#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV)                    \
  static cl::opt<std::string> NAME##TablePath(                                 \
      CMD, cl::desc("The output-path to the " #NAME " table"), cl::init(CSV),  \
      cl::cat(PTABenCat));
#include "SupportedAnalysisTypes.def"

using psr::ptaben::SupportedAnalysisTypes;

static constexpr psr::UnionFindAliasAnalysisType
ufaaTypeFromSupported(SupportedAnalysisTypes AT) {
  switch (AT) {
  case SupportedAnalysisTypes::CFLAnders:
  case SupportedAnalysisTypes::CFLSteens:
  case SupportedAnalysisTypes::AndersOTF:
  case SupportedAnalysisTypes::AndersOTFCtxDyn:
  case SupportedAnalysisTypes::AndersOTFCtxAll:
    llvm::report_fatal_error("Not a union-find analysis");
  case SupportedAnalysisTypes::UFAACtx:
    return psr::UnionFindAliasAnalysisType::CtxSens;
  case SupportedAnalysisTypes::UFAAInd:
    return psr::UnionFindAliasAnalysisType::IndSens;
  case SupportedAnalysisTypes::UFAACtxInd:
    return psr::UnionFindAliasAnalysisType::CtxIndSens;
  case SupportedAnalysisTypes::UFAABotCtx:
    return psr::UnionFindAliasAnalysisType::BotCtxSens;
  case SupportedAnalysisTypes::UFAABotCtxInd:
    return psr::UnionFindAliasAnalysisType::BotCtxIndSens;
  }
}

static psr::AliasResult
checkLLVMQueryLoc(psr::LLVMAliasInfoRef ComputedAliasResult,
                  const llvm::Instruction *QueryInst) {
  const auto *Ptr1 = QueryInst->getOperand(0);
  const auto *Ptr2 = QueryInst->getOperand(1);

  return ComputedAliasResult.alias(Ptr1, Ptr2, QueryInst);
}

template <typename AAResT>
static psr::AliasResult
checkLLVMQueryLoc(psr::LLVMRawAliasIterator<AAResT> &ComputedAliasResult,
                  const llvm::Instruction *QueryInst) {
  const auto *Ptr1 = QueryInst->getOperand(0);
  const auto *Ptr2 = QueryInst->getOperand(1);

  return ComputedAliasResult.alias(Ptr1, Ptr2, QueryInst);
}

static void
performAndersAnalysis(psr::LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
                      auto &&RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLAnders);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void
performSteensAnalysis(psr::LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
                      auto &&RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLSteens);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performUnionFindAliasAnalysis(
    psr::LLVMProjectIRDB &IRDB, const psr::LLVMBasedCallGraph &BaseCG,
    llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs, auto &&RC,
    psr::UnionFindAliasAnalysisType AType) {
  auto AliasSet = psr::LLVMUnionFindAliasSet(
      &IRDB, BaseCG,
      psr::LLVMUnionFindAliasSet::Config{
          .AType = AType,
          .ALocality = psr::LLVMUnionFindAliasSet::AnalysisLocality::Global,
      });

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void
performAndersenOTFAA(psr::LLVMProjectIRDB &IRDB,
                     llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
                     auto &&RC, psr::ContextSensitivityOptions::Mode CtxMode) {
  auto EntryFunctions =
      getEntryFunctions(IRDB, psr::getDefaultEntryPoints(IRDB));
  auto VC = psr::ValueCompressor<psr::PAGVariable>();
  psr::ContextSensitivityOptions CSOpts;
  CSOpts.SelectionMode = CtxMode;
  auto AARes = psr::computeAndersenOTF(
      IRDB, EntryFunctions, &VC, psr::Soundness::Soundy, std::move(CSOpts));

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(AARes, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void
performAnalysis(psr::LLVMProjectIRDB &IRDB,
                const psr::LLVMBasedCallGraph &BaseCG,
                llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs, auto &&RC,
                SupportedAnalysisTypes AType) {
  switch (AType) {
  case SupportedAnalysisTypes::CFLAnders:
    return performAndersAnalysis(IRDB, QueryLocs, PSR_FWD(RC));
  case SupportedAnalysisTypes::CFLSteens:
    return performSteensAnalysis(IRDB, QueryLocs, PSR_FWD(RC));
  case SupportedAnalysisTypes::UFAACtx:
  case SupportedAnalysisTypes::UFAAInd:
  case SupportedAnalysisTypes::UFAACtxInd:
  case SupportedAnalysisTypes::UFAABotCtx:
  case SupportedAnalysisTypes::UFAABotCtxInd:
    return performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, PSR_FWD(RC),
                                         ufaaTypeFromSupported(AType));
  case SupportedAnalysisTypes::AndersOTF:
    return performAndersenOTFAA(IRDB, QueryLocs, PSR_FWD(RC),
                                psr::ContextSensitivityOptions::Mode::Off);
  case SupportedAnalysisTypes::AndersOTFCtxDyn:
    return performAndersenOTFAA(IRDB, QueryLocs, PSR_FWD(RC),
                                psr::ContextSensitivityOptions::Mode::Dynamic);
  case SupportedAnalysisTypes::AndersOTFCtxAll:
    return performAndersenOTFAA(IRDB, QueryLocs, PSR_FWD(RC),
                                psr::ContextSensitivityOptions::Mode::All);
  }
}

static auto openFileOrExit(llvm::StringRef Filepath) {
  auto File = psr::openFileForWrite(Filepath);
  if (!File) {
    std::exit(1);
  }
  return File;
}

static int checkSingleFile() {
  llvm::WithColor::note() << "Analyzing " << IRFilePath << '\n';

  auto IRDB = psr::LLVMProjectIRDB::loadOrExit(IRFilePath);
  auto *Mod = IRDB.getModule();
  assert(Mod != nullptr);
  llvm::SmallVector<psr::ptaben::QueryLocation, 4> QueryLocs;
  llvm::SmallVector<psr::ptaben::QuerySrcCodeLocation> QuerySrcLocs;
  psr::ptaben::findAllQueryLocations(*Mod, QueryLocs, &QuerySrcLocs);

  if (QueryLocs.empty()) {
    llvm::WithColor::warning()
        << "File does not contain any alias queries. Skip it.\n";
    return true;
  }

  struct ResultEntry {
    psr::AliasResult Result;
    uint32_t Align;
  };
  llvm::SmallDenseMap<psr::ptaben::QueryId,
                      std::map<llvm::StringRef, ResultEntry>>
      ResultTable;
  struct ResEntryCollector {
    llvm::StringRef Analysis;
    llvm::SmallDenseMap<psr::ptaben::QueryId,
                        std::map<llvm::StringRef, ResultEntry>> &Res; // NOLINT

    void handleResult(psr::ptaben::PTAResult Result) {
      Res[Result.Query][Analysis] = ResultEntry{
          .Result = Result.Result,
          .Align = uint32_t(Analysis.size()),
      };
    }
  };

  auto VTP = psr::LLVMVFTableProvider(IRDB);
  auto TH = psr::DIBasedTypeHierarchy(IRDB);
  auto RTARes = psr::RTAResolver(&IRDB, &VTP, &TH);
  const auto BaseCG = buildLLVMBasedCallGraph(
      IRDB, RTARes, getEntryFunctions(IRDB, psr::getDefaultEntryPoints(IRDB)));

  for (auto AType : psr::ptaben::AllSupportedAnalysisTypes) {
    performAnalysis(
        IRDB, BaseCG, QueryLocs,
        ResEntryCollector{.Analysis = to_string(AType), .Res = ResultTable},
        AType);
  }

  llvm::outs() << "QueryId,\t\tQuery,  \t";

  llvm::interleaveComma(ResultTable.begin()->second, llvm::outs(),
                        [](const auto &Entry) { llvm::outs() << Entry.first; });
  llvm::outs() << '\n';

  for (const auto &[QId, QRes] : ResultTable) {
    auto *QType = llvm::find_if(
        QueryLocs, [&](const auto &QLoc) { return QLoc.Id == QId; });
    assert(QType != nullptr);
    llvm::outs() << uint64_t(QId) << ",\t" << to_string(QType->QueryType)
                 << ",\t";

    size_t Last = QRes.size() - 1;
    size_t Ctr = 0;
    for (const auto &Entry : QRes) {
      auto Str = to_string(Entry.second.Result);
      llvm::outs() << Str;

      if (Ctr++ != Last) {
        auto Len = Str.size();
        auto Align = Entry.second.Align;
        auto Diff = -(Len < Align) & (Align - Len);

        llvm::outs() << ',';
        llvm::outs().indent(Diff) << ' ';
      }
    }
    llvm::outs() << '\n';
  }

  return 0;
}

static int performCompleteExperiment() {
  auto QFile = openFileOrExit(QueryTablePath);
  psr::ptaben::QuerySerializer QSer(QFile.get());

  psr::TypedArray<SupportedAnalysisTypes, std::unique_ptr<llvm::raw_ostream>,
                  psr::ptaben::NumSupportedAnalysisTypes>
      ResultFiles;

#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV)                    \
  ResultFiles[SupportedAnalysisTypes::NAME] = openFileOrExit(NAME##TablePath);
#include "SupportedAnalysisTypes.def"

  psr::TypedArray<SupportedAnalysisTypes, psr::ptaben::ResultCollector,
                  psr::ptaben::NumSupportedAnalysisTypes>
      ResultSer{psr::generate_tag, [&](auto AType) {
                  return psr::ptaben::ResultCollector(ResultFiles[AType].get(),
                                                      to_string(AType));
                }};

  llvm::SmallVector<std::string, 4> Failures;

  psr::ptaben::checkDirs(IRPaths, Failures, [&](llvm::StringRef FileName) {
    llvm::WithColor::note() << "Analyzing " << FileName << '\n';

    auto IRDB = psr::LLVMProjectIRDB::loadOrExit(FileName);
    auto *Mod = IRDB.getModule();
    assert(Mod != nullptr);

    llvm::SmallVector<psr::ptaben::QueryLocation, 4> QueryLocs;
    llvm::SmallVector<psr::ptaben::QuerySrcCodeLocation> QuerySrcLocs;
    psr::ptaben::findAllQueryLocations(*Mod, QueryLocs, &QuerySrcLocs);

    if (QueryLocs.empty()) {
      llvm::WithColor::warning()
          << "File does not contain any alias queries. Skip it.\n";
      return true;
    }

    for (const auto &[QLoc, QSrcLoc] : llvm::zip(QueryLocs, QuerySrcLocs)) {
      QSer.handleQuery(QLoc, QSrcLoc);
    }

    auto VTP = psr::LLVMVFTableProvider(IRDB);
    auto TH = psr::DIBasedTypeHierarchy(IRDB);
    auto Res = psr::RTAResolver(&IRDB, &VTP, &TH);
    const auto BaseCG = buildLLVMBasedCallGraph(
        IRDB, Res, getEntryFunctions(IRDB, psr::getDefaultEntryPoints(IRDB)));
    for (const auto &[AType, ASer] : ResultSer.enumerate()) {
      performAnalysis(IRDB, BaseCG, QueryLocs, ASer, AType);
    }

    return true;
  });

  return 0;
}

int main(int Argc, char *Argv[]) {
  cl::HideUnrelatedOptions(PTABenCat);
  cl::ParseCommandLineOptions(Argc, Argv);

  if (CheckFileCmd) {
    return checkSingleFile();
  }
  return performCompleteExperiment();
}
