#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/CallGraphBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/Pointer/BottomupUnionFindAA.h"
#include "phasar/Pointer/PointerAssignmentGraph.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <map>
#include <vector>

namespace {
using namespace psr;

constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("pointers/");

[[nodiscard]] ValueId asId(const ValueCompressor<PAGVariable> &VC,
                           const LLVMProjectIRDB &IRDB,
                           unittest::TestingSrcLocation Var) {

  const auto *LLVMVar = unittest::testingLocInIR(Var, IRDB);
  return VC.getOrNull(LLVMVar).value();
}

[[nodiscard]] RawAliasSet<ValueId>
asIdBased(const ValueCompressor<PAGVariable> &VC, const LLVMProjectIRDB &IRDB,
          llvm::ArrayRef<unittest::TestingSrcLocation> AliasSet) {
  RawAliasSet<ValueId> Ret;

  for (auto Var : AliasSet) {
    const auto *LLVMVar = unittest::testingLocInIR(Var, IRDB);
    Ret.insert(VC.getOrNull(LLVMVar).value());
  }

  return Ret;
}

[[nodiscard]] std::string stringifyVal(const ValueCompressor<PAGVariable> &VC,
                                       ValueId Id) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);

  ROS << "{ ";

  const auto &LLVMVars = VC.id2vars(Id);
  llvm::interleaveComma(LLVMVars, ROS, [&](PAGVariable Var) {
    // XXX: Should we map back to TestingSrcLocation?
    ROS << to_string(Var);
  });

  ROS << " }";

  return Ret;
}

using GTMap = std::map<unittest::TestingSrcLocation,
                       std::vector<unittest::TestingSrcLocation>>;

void doAnalysisAndCompareResults(
    const llvm::Twine &IRFile, const GTMap &ExpectedResults,
    std::invocable<const LLVMProjectIRDB &, const LLVMBasedCallGraph &> auto
        AABuilder) {

  auto IRDB = LLVMProjectIRDB::loadOrExit(PathToLLFiles + IRFile);
  auto TH = DIBasedTypeHierarchy(IRDB);
  auto VTP = LLVMVFTableProvider(IRDB);
  auto BaseCG = buildLLVMBasedCallGraph(IRDB, CallGraphAnalysisType::RTA,
                                        {"main"}, TH, VTP);

  ValueCompressor<PAGVariable> VC;
  auto PB = LLVMPAGBuilder();
  auto AA = AABuilder(IRDB, BaseCG);
  PB.buildPAG(IRDB, VC, &AA);

  UnionFindAAResult auto Results = std::move(AA).consumeAAResults(VC.size());

  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    const auto PtrId = asId(VC, IRDB, PtrVar);
    const auto ExpectedAliasIds = asIdBased(VC, IRDB, ExpectedAliasVars);

    const RawAliasSet<ValueId> &ComputedAliasIds =
        Results.getRawAliasSet(PtrId);

    ExpectedAliasIds.foreach ([&](ValueId VId) {
      if (!ComputedAliasIds.contains(VId)) {
        ADD_FAILURE() << "Did not compute expected alias of " << PtrVar << ": "
                      << stringifyVal(VC, VId);
      }
    });

    ComputedAliasIds.foreach ([&](ValueId VId) {
      if (ExpectedAliasIds.contains(VId)) {
        return;
      }

      const auto &Vars = VC.id2vars(VId);
      if (llvm::any_of(Vars, [](PAGVariable Var) {
            if (Var.isReturnVariable()) {
              return true;
            }

            if (llvm::isa<llvm::LoadInst>(Var.valueOrNull())) {
              return true;
            }

            return false;
          })) {
        return;
      }
      ADD_FAILURE() << "Computed unexpected alias of " << PtrVar << ": "
                    << stringifyVal(VC, VId);
    });
  }

  if (::testing::Test::HasFailure()) {
    Results.dump();
  }
}

using namespace psr::unittest;

constexpr auto ContextAABuilder = [](const auto &IRDB, const auto &CG) {
  return CallingContextSensUnionFindAA<LLVMPAGDomain>{
      &CG,
      &IRDB,
  };
};

constexpr auto IndAABuilder = [](const auto & /*IRDB*/, const auto &CG) {
  auto Ret = pag::PBMixin{
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
      pag::LLVMCGProvider{&CG},
  };

  static_assert(pag::PBStrategy<decltype(Ret)>);
  return Ret;
};

constexpr auto BotAABuilder = [](const auto &IRDB, const auto &CG) {
  auto Ret = BottomupUnionFindAA<LLVMPAGDomain>{
      ReverseCGGraph{
          &CG,
          &IRDB,
      },
  };

  static_assert(pag::PBStrategy<decltype(Ret)>);
  return Ret;
};

TEST(CtxSensUnionFindAATest, Basic01) {
  GTMap GT = {{LineColFunOp{.Line = 3,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 3,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 5,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};
  doAnalysisAndCompareResults("basic_01_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Basic02) {
  GTMap GT = {{LineColFunOp{.Line = 3,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 3,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 6,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("basic_02_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Basic03) {
  GTMap GT = {{LineColFunOp{.Line = 4,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 4,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 6,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("basic_03_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context01) {
  GTMap GT = {
      {LineColFunOp{.Line = 5,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 5,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 8,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 11,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
           ArgInFun{.Idx = 0, .InFunction = "id"},
       }},
      {LineColFunOp{.Line = 6,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 6,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 9,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           ArgInFun{.Idx = 0, .InFunction = "id"},
       }},
      {ArgInFun{.Idx = 0, .InFunction = "id"},
       {
           LineColFunOp{.Line = 5,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 6,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 8,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 9,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 11,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
           ArgInFun{.Idx = 0, .InFunction = "id"},
       }},
  };
  doAnalysisAndCompareResults("context_01_c_dbg.ll", GT, ContextAABuilder);
}

// TODO: Add more context tests

// TODO: Add tests for IndirectionSensUnionFindAA

TEST(BotUnionFindAATest, Context01) {
  GTMap GT = {
      {LineColFunOp{.Line = 5,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 5,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 8,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 11,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
       }},
      {LineColFunOp{.Line = 6,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 6,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 9,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
       }},
  };

  doAnalysisAndCompareResults("context_01_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context02) {
  GTMap GT = {
      {LineColFunOp{.Line = 6,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 6,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 9,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 10,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 14,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
       }},
      {LineColFunOp{.Line = 7,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 7,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 11,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 12,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
       }},
  };

  doAnalysisAndCompareResults("context_02_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context03) {
  GTMap GT = {{LineColFunOp{.Line = 6,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 6,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 8,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 9,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 11,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_03_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context04) {
  GTMap GT = {{LineColFunOp{.Line = 7,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 7,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 9,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 10,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_04_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context05) {
  GTMap GT = {{LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 8,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 10,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 11,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 13,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_05_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context06) {
  GTMap GT = {{LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 9,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 11,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 14,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_06_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context07) {
  GTMap GT = {
      {LineColFunOp{.Line = 8,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 8,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 11,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 14,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
       }},
      {LineColFunOp{.Line = 9,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 9,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 12,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
       }},
  };
  // psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
  //                                     "BottomupUnionFindAA");
  doAnalysisAndCompareResults("context_07_c_dbg.ll", GT, BotAABuilder);
}

// TODO: Add more tests for BottomupUnionFindAA

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
