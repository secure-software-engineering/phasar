#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/IO.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "PTAUtils.h"
#include "QuerySer.h"
#include "ResultsCollector.h"

#include <string>

namespace cl = llvm::cl;

static cl::OptionCategory PTABenCat("PTABen Benhchmark Tool");

static cl::opt<std::string> IRPath(cl::Positional, cl::Required,
                                   cl::desc("ptaben-ir-directory"),
                                   cl::cat(PTABenCat));
static cl::opt<std::string>
    QueryTablePath("queries-table",
                   cl::desc("The Output-Path to the queries table"),
                   cl::init("queries.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    AndersTablePath("anders-table",
                    cl::desc("The Output-Path to the anders output table"),
                    cl::init("anders-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    SteensTablePath("steens-table",
                    cl::desc("The Output-Path to the steens output table"),
                    cl::init("steens-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    CtxTablePath("ctx-table",
                 cl::desc("The Output-Path to the ctx output table"),
                 cl::init("ctx-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    BotTablePath("bot-table",
                 cl::desc("The Output-Path to the bot output table"),
                 cl::init("bot-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    IndTablePath("ind-table",
                 cl::desc("The Output-Path to the ind output table"),
                 cl::init("ind-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string>
    CtxIndTablePath("ctx-ind-table",
                    cl::desc("The Output-Path to the ctx-ind output table"),
                    cl::init("ctx-ind-results.csv"), cl::cat(PTABenCat));
static cl::opt<std::string> BotCtxIndTablePath(
    "bot-ctx-ind-table",
    cl::desc("The Output-Path to the bot-ctx-ind output table"),
    cl::init("bot-ctx-ind-results.csv"), cl::cat(PTABenCat));

static psr::AliasResult
checkLLVMQueryLoc(psr::LLVMAliasInfoRef ComputedAliasResult,
                  const llvm::Instruction *QueryInst) {
  const auto *Ptr1 = QueryInst->getOperand(0);
  const auto *Ptr2 = QueryInst->getOperand(1);

  return ComputedAliasResult.alias(Ptr1, Ptr2, QueryInst);
}

static void
performAndersAnalysis(psr::LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
                      psr::ptaben::ResultCollector &RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLAnders);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void
performSteensAnalysis(psr::LLVMProjectIRDB &IRDB,
                      llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
                      psr::ptaben::ResultCollector &RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLSteens);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::ptaben::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performUnionFindAliasAnalysis(
    psr::LLVMProjectIRDB &IRDB, const psr::LLVMBasedCallGraph &BaseCG,
    llvm::ArrayRef<psr::ptaben::QueryLocation> QueryLocs,
    psr::ptaben::ResultCollector &RC, psr::UnionFindAliasAnalysisType AType) {
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

static auto openFileOrExit(llvm::StringRef Filepath) {
  auto File = psr::openFileForWrite(Filepath);
  if (!File) {
    std::exit(1);
  }
  return File;
}

int main(int Argc, char *Argv[]) {
  cl::HideUnrelatedOptions(PTABenCat);
  cl::ParseCommandLineOptions(Argc, Argv);

  auto QFile = openFileOrExit(QueryTablePath);
  auto AndersFile = openFileOrExit(AndersTablePath);
  auto SteensFile = openFileOrExit(SteensTablePath);
  auto CtxFile = openFileOrExit(CtxTablePath);
  auto BotFile = openFileOrExit(BotTablePath);
  auto IndFile = openFileOrExit(IndTablePath);
  auto CtxIndFile = openFileOrExit(CtxIndTablePath);
  auto BotCtxIndFile = openFileOrExit(BotCtxIndTablePath);

  psr::ptaben::QuerySerializer QSer(QFile.get());
  psr::ptaben::ResultCollector AndersSer(AndersFile.get(), "AndersResult");
  psr::ptaben::ResultCollector SteensSer(SteensFile.get(), "SteensResult");
  psr::ptaben::ResultCollector CtxSer(CtxFile.get(), "CtxResult");
  psr::ptaben::ResultCollector BotSer(BotFile.get(), "BotResult");
  psr::ptaben::ResultCollector IndSer(IndFile.get(), "IndResult");
  psr::ptaben::ResultCollector CtxIndSer(CtxIndFile.get(), "CtxIndResult");
  psr::ptaben::ResultCollector BotCtxIndSer(BotCtxIndFile.get(),
                                            "BotCtxIndResult");

  llvm::SmallVector<std::string, 4> Failures;
  psr::ptaben::checkDir(IRPath, Failures, [&](llvm::StringRef FileName) {
    llvm::errs() << "Analyzing " << FileName << '\n';

    psr::LLVMProjectIRDB IRDB(FileName);
    auto *Mod = IRDB.getModule();
    if (!Mod) {
      return false;
    }

    llvm::SmallVector<psr::ptaben::QueryLocation, 4> QueryLocs;
    llvm::SmallVector<psr::ptaben::QuerySrcCodeLocation> QuerySrcLocs;
    psr::ptaben::findAllQueryLocations(*Mod, QueryLocs, &QuerySrcLocs);

    if (QueryLocs.empty()) {
      llvm::errs()
          << "[NOTE]: File does not contain any alias queries. Skip it.\n";
      return true;
    }

    for (const auto &[QLoc, QSrcLoc] : llvm::zip(QueryLocs, QuerySrcLocs)) {
      QSer.handleQuery(QLoc, QSrcLoc);
    }

    using psr::UnionFindAliasAnalysisType;

    performAndersAnalysis(IRDB, QueryLocs, AndersSer);
    performSteensAnalysis(IRDB, QueryLocs, SteensSer);

    auto VTP = psr::LLVMVFTableProvider(IRDB);
    auto TH = psr::DIBasedTypeHierarchy(IRDB);
    auto Res = psr::RTAResolver(&IRDB, &VTP, &TH);
    const auto BaseCG = buildLLVMBasedCallGraph(
        IRDB, Res, getEntryFunctions(IRDB, psr::getDefaultEntryPoints(IRDB)));

    performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, CtxSer,
                                  UnionFindAliasAnalysisType::CtxSens);
    performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, BotSer,
                                  UnionFindAliasAnalysisType::BotCtxSens);
    performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, IndSer,
                                  UnionFindAliasAnalysisType::IndSens);
    performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, CtxIndSer,
                                  UnionFindAliasAnalysisType::CtxIndSens);
    performUnionFindAliasAnalysis(IRDB, BaseCG, QueryLocs, BotCtxIndSer,
                                  UnionFindAliasAnalysisType::BotCtxIndSens);

    return true;
  });
}
