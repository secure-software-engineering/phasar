#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/BottomupUnionFindAA.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/FileUtils.hpp"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/PTAUtils.hpp"
#include "phasar/Utils/QuerySer.hpp"
#include "phasar/Utils/ResultsCollector.hpp"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace cl = llvm::cl;

static cl::opt<std::string> IRPath(cl::Positional, cl::Required,
                                   cl::desc("ptaben-ir-directory"));
static cl::opt<std::string>
    QueryTablePath("queries-table",
                   cl::desc("The Output-Path to the queries table"),
                   cl::init("queries.csv"));
static cl::opt<std::string>
    AndersTablePath("anders-table",
                    cl::desc("The Output-Path to the anders output table"),
                    cl::init("anders-results.csv"));
static cl::opt<std::string>
    SteensTablePath("steens-table",
                    cl::desc("The Output-Path to the steens output table"),
                    cl::init("steens-results.csv"));
static cl::opt<std::string>
    CtxTablePath("ctx-table",
                 cl::desc("The Output-Path to the ctx output table"),
                 cl::init("ctx-results.csv"));
static cl::opt<std::string>
    BotTablePath("bot-table",
                 cl::desc("The Output-Path to the bot output table"),
                 cl::init("bot-results.csv"));
static cl::opt<std::string>
    IndTablePath("ind-table",
                 cl::desc("The Output-Path to the ind output table"),
                 cl::init("ind-results.csv"));

constexpr auto CtxAABuilder = [](const auto &IRDB, const auto &CG) {
  return psr::CallingContextSensUnionFindAA<psr::LLVMPAGDomain>{
      &CG,
      &IRDB,
  };
};

constexpr auto IndAABuilder = [](const auto & /*IRDB*/, const auto &CG) {
  auto Ret = psr::pag::PBMixin{
      psr::IndirectionSensUnionFindAA<psr::LLVMPAGDomain>{},
      psr::pag::LLVMCGProvider{&CG},
  };

  static_assert(psr::pag::PBStrategy<decltype(Ret)>);
  return Ret;
};

constexpr auto BotAABuilder = [](const auto &IRDB, const auto &CG) {
  auto Ret = psr::BottomupUnionFindAA<psr::LLVMPAGDomain>{
      psr::ReverseCGGraph{
          &CG,
          &IRDB,
      },
  };

  static_assert(psr::pag::PBStrategy<decltype(Ret)>);
  return Ret;
};

static psr::AliasResult mapAliasResult(psr::AliasResult Res) {
  switch (Res) {
  case psr::AliasResult::NoAlias:
    return psr::AliasResult::NoAlias;

  case psr::AliasResult::PartialAlias:
  case psr::AliasResult::MayAlias:
  case psr::AliasResult::MustAlias:
    return psr::AliasResult::MayAlias;
  }
  llvm_unreachable(
      "All AliasResult kinds should be handled in the switch above!");
}

static psr::AliasResult
checkLLVMQueryLoc(psr::LLVMAliasInfoRef ComputeAliasResult,
                  const llvm::Instruction *QueryInst) {
  const auto *Ptr1 = QueryInst->getOperand(0);
  const auto *Ptr2 = QueryInst->getOperand(1);

  return mapAliasResult(ComputeAliasResult.alias(Ptr1, Ptr2, QueryInst));
}

static void performAndersAnalysis(psr::LLVMProjectIRDB &IRDB,
                                  llvm::ArrayRef<psr::QueryLocation> QueryLocs,
                                  psr::ResultCollector &RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLAnders);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performSteensAnalysis(psr::LLVMProjectIRDB &IRDB,
                                  llvm::ArrayRef<psr::QueryLocation> QueryLocs,
                                  psr::ResultCollector &RC) {
  psr::LLVMAliasSet AliasSet(&IRDB, false, psr::AliasAnalysisType::CFLSteens);

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performCtxAnalysis(psr::LLVMProjectIRDB &IRDB,
                               llvm::ArrayRef<psr::QueryLocation> QueryLocs,
                               psr::ResultCollector &RC) {
  psr::LLVMUnionFindAliasSet AliasSet(
      &IRDB,
      psr::LLVMUnionFindAliasSet::Config{
          .AType = psr::UnionFindAliasAnalysisType::CtxSens,
          .ALocality = psr::LLVMUnionFindAliasSet::AnalysisLocality::Global,
      });

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performBotAnalysis(psr::LLVMProjectIRDB &IRDB,
                               llvm::ArrayRef<psr::QueryLocation> QueryLocs,
                               psr::ResultCollector &RC) {
  psr::LLVMUnionFindAliasSet AliasSet(
      &IRDB,
      psr::LLVMUnionFindAliasSet::Config{
          .AType = psr::UnionFindAliasAnalysisType::BotCtxSens,
          .ALocality = psr::LLVMUnionFindAliasSet::AnalysisLocality::Global,
      });

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

static void performIndAnalysis(psr::LLVMProjectIRDB &IRDB,
                               llvm::ArrayRef<psr::QueryLocation> QueryLocs,
                               psr::ResultCollector &RC) {
  psr::LLVMUnionFindAliasSet AliasSet(
      &IRDB,
      psr::LLVMUnionFindAliasSet::Config{
          .AType = psr::UnionFindAliasAnalysisType::IndSens,
          .ALocality = psr::LLVMUnionFindAliasSet::AnalysisLocality::Global,
      });

  for (const auto &Loc : QueryLocs) {
    auto Res = checkLLVMQueryLoc(&AliasSet, Loc.Inst);
    RC.handleResult(psr::PTAResult{.Query = Loc.Id, .Result = Res});
  }
}

int main(int Argc, char *Argv[]) {
  cl::ParseCommandLineOptions(Argc, Argv);

  auto QFile = psr::openFileForWrite(QueryTablePath);
  if (!QFile) {
    return 1;
  }

  auto AndersFile = psr::openFileForWrite(AndersTablePath);
  if (!AndersFile) {
    return 1;
  }

  auto SteensFile = psr::openFileForWrite(SteensTablePath);
  if (!SteensFile) {
    return 1;
  }

  auto CtxFile = psr::openFileForWrite(CtxTablePath);
  if (!CtxFile) {
    return 1;
  }

  auto BotFile = psr::openFileForWrite(BotTablePath);
  if (!BotFile) {
    return 1;
  }

  auto IndFile = psr::openFileForWrite(IndTablePath);
  if (!IndFile) {
    return 1;
  }

  psr::QuerySerializer QSer(QFile.get());
  psr::ResultCollector AndersSer(AndersFile.get(), "AndersResult");
  psr::ResultCollector SteensSer(SteensFile.get(), "SteensResult");
  psr::ResultCollector CtxSer(CtxFile.get(), "CtxResult");
  psr::ResultCollector BotSer(BotFile.get(), "BotResult");
  psr::ResultCollector IndSer(IndFile.get(), "IndResult");

  llvm::SmallVector<std::string, 4> Failures;
  psr::checkDir(IRPath, Failures, [&](llvm::StringRef FileName) {
    llvm::errs() << "Analyzing " << FileName << '\n';

    psr::LLVMProjectIRDB IRDB(FileName);
    auto *Mod = IRDB.getModule();
    if (!Mod) {
      return false;
    }

    llvm::SmallVector<psr::QueryLocation, 4> QueryLocs;
    llvm::SmallVector<psr::QuerySrcCodeLocation> QuerySrcLocs;
    psr::findAllQueryLocations(*Mod, QueryLocs, &QuerySrcLocs);

    if (QueryLocs.empty()) {
      llvm::errs()
          << "[NOTE]: File does not contain any alias queries. Skip it.\n";
      return true;
    }

    for (const auto &[QLoc, QSrcLoc] : llvm::zip(QueryLocs, QuerySrcLocs)) {
      QSer.handleQuery(QLoc, QSrcLoc);
    }

    performAndersAnalysis(IRDB, QueryLocs, AndersSer);
    performSteensAnalysis(IRDB, QueryLocs, SteensSer);
    performCtxAnalysis(IRDB, QueryLocs, CtxSer);
    performBotAnalysis(IRDB, QueryLocs, BotSer);
    performIndAnalysis(IRDB, QueryLocs, IndSer);

    return true;
  });
}
