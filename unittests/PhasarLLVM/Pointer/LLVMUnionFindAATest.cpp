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
#include <source_location>
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
        AABuilder,
    std::source_location Loc = std::source_location::current()) {

  auto IRDB = LLVMProjectIRDB::loadOrExit(PathToLLFiles + IRFile);
  auto TH = DIBasedTypeHierarchy(IRDB);
  auto VTP = LLVMVFTableProvider(IRDB);
  auto BaseCG = buildLLVMBasedCallGraph(IRDB, CallGraphAnalysisType::RTA,
                                        {"main"}, TH, VTP);

  ValueCompressor<PAGVariable> VC;
  auto AA = AABuilder(IRDB, BaseCG);

  UnionFindAAResult auto Results =
      computeUnionFindAARaw(IRDB, std::move(AA), &VC);

  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    const auto PtrId = asId(VC, IRDB, PtrVar);
    const auto ExpectedAliasIds = asIdBased(VC, IRDB, ExpectedAliasVars);

    const RawAliasSet<ValueId> &ComputedAliasIds =
        Results.getRawAliasSet(PtrId);

    ExpectedAliasIds.foreach ([&](ValueId VId) {
      if (!ComputedAliasIds.contains(VId)) {
        ADD_FAILURE_AT(Loc.file_name(), Loc.line())
            << "Did not compute expected alias of " << PtrVar << ": "
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
      ADD_FAILURE_AT(Loc.file_name(), Loc.line())
          << "Computed unexpected alias of " << PtrVar << ": "
          << stringifyVal(VC, VId);
    });
  }

  if (::testing::Test::HasFailure()) {
    llvm::errs() << "ValueCompressor: {\n";
    for (const auto &[VId, Values] : VC.id2vars().enumerate()) {
      llvm::errs() << "  #" << uint32_t(VId) << ":\n";
      for (const auto Val : Values) {
        llvm::errs() << "    " << to_string(Val) << '\n';
      }
    }
    llvm::errs() << "}\n";
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
  /*

  Note: only relevant lines were pasted here

  ValueCompressor:
  #1:
    @"_ZTIZ4mainE3$_0" = internal constant { ptr, ptr } { ptr getelementptr
  inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr
  @"_ZTSZ4mainE3$_0" }, align 8, !psr.id !2 | ID: 2
  #11:
    %0 = load ptr, ptr %FuncPtr, align 8, !dbg !415, !psr.id !416 | ID: 13
  #46:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !538, !psr.id !550 | ID: 76

  --- ---

  #1: <6, 10, 21, 22, 24, 56, 58, 88>
  #11: <8, 15, 31, 33, 160, 161, 163, 181, 183, 209>
  #46: <46, 47, 59, 141, 184, 253>

  --- for #1: <6, 10, 21, 22, 24, 56, 58, 88> ---

  #6:
    %ref.tmp = alloca %class.anon, align 1, !psr.id !402 | ID: 6
  #10:
    ptr %__f | ID: _ZNSt8functionIFivEEC2IZ4mainE3$_0vEEOT_.1
  #21:
    %1 = load ptr, ptr %__f.addr, align 8, !dbg !448, !psr.id !451 | ID: 29
    %2 = load ptr, ptr %__f.addr, align 8, !dbg !459, !psr.id !460 | ID: 33
  #22:
    ptr %0 | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_0E21_M_not_empty_functionIS1_EEbRKT_.0
  #24:
    ptr %__f | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_0E15_M_init_functorIS1_EEvRSt9_Any_dataOT_.1
  #56:
    %1 = load ptr, ptr %__f.addr, align 8, !dbg !647, !psr.id !648 | ID: 103
  #58:
    ptr %__f | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_0E9_M_createIS1_EEvRSt9_Any_dataOT_St17integral_constantIbLb1EE.1
  #88:
    %2 = load ptr, ptr %__f.addr, align 8, !dbg !752, !psr.id !753 | ID: 154

  --- for #11: <8, 15, 31, 33, 160, 161, 163, 181, 183, 209> ---

  #8:
    %ref.tmp1 = alloca %class.anon.0, align 1, !psr.id !404 | ID: 8
  #15:
    ptr %__f | ID:
  _ZNSt8functionIFivEEaSIZ4mainE3$_1EENSt9enable_ifIXsrNS1_9_CallableIT_NS4_IXntsr7is_sameINSt9remove_cvINSt16remove_referenceIS6_E4typeEE4typeES1_EE5valueESt5decayIS6_EE4type4typeESt15__invoke_resultIRSH_JEEEE5valueERS1_E4typeEOS6_.1
  #31:
    %0 = load ptr, ptr %__f.addr, align 8, !dbg !500, !psr.id !501 | ID: 49
  #33:
    ptr %__f | ID: _ZNSt8functionIFivEEC2IZ4mainE3$_1vEEOT_.1
  #160:
    %1 = load ptr, ptr %__f.addr, align 8, !dbg !1063, !psr.id !1066 | ID: 300
    %2 = load ptr, ptr %__f.addr, align 8, !dbg !1074, !psr.id !1075 | ID: 304
  #161:
    ptr %0 | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_1E21_M_not_empty_functionIS1_EEbRKT_.0
  #163:
    ptr %__f | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_1E15_M_init_functorIS1_EEvRSt9_Any_dataOT_.1
  #181:
    %1 = load ptr, ptr %__f.addr, align 8, !dbg !1170, !psr.id !1171 | ID: 344
  #183:
    ptr %__f | ID:
  _ZNSt14_Function_base13_Base_managerIZ4mainE3$_1E9_M_createIS1_EEvRSt9_Any_dataOT_St17integral_constantIbLb1EE.1
  #209:
    %2 = load ptr, ptr %__f.addr, align 8, !dbg !1275, !psr.id !1276 | ID: 395

  --- for #46: <46, 47, 59, 141, 184, 253> ---

  #46:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !538, !psr.id !550 | ID: 76
  #47:
    ptr %0 | ID: __clang_call_terminate.0
  #59:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !649, !psr.id !654 | ID: 107
  #141:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !976, !psr.id !981 | ID: 259
  #184:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !1172, !psr.id !1177 | ID: 348
  #253:
    %3 = extractvalue { ptr, i32 } %2, 0, !dbg !1458, !psr.id !1463 | ID: 482

  */
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

TEST(CtxSensUnionFindAATest, Context02) {
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

  doAnalysisAndCompareResults("context_02_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context03) {
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

  doAnalysisAndCompareResults("context_03_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context04_0) {
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

  doAnalysisAndCompareResults("context_04_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context04_1) {
  GTMap GT = {{LineColFunOp{.Line = 7,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 7,
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
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_04_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context05_0) {
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

  doAnalysisAndCompareResults("context_05_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context05_1) {
  GTMap GT = {{LineColFunOp{.Line = 8,
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
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 16,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_05_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context06_0) {
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

  doAnalysisAndCompareResults("context_06_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context06_1) {
  GTMap GT = {{LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 10,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 14,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 17,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_06_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context07) {
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

  doAnalysisAndCompareResults("context_07_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context08) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 19,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 21,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_08_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context09_0) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 16,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_09_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context09_1) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 17,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 20,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 22,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_09_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context10_0) {
  GTMap GT = {{LineColFunOp{.Line = 24,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 24,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 26,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 30,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 32,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_10_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context10_1) {
  GTMap GT = {{LineColFunOp{.Line = 24,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 24,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 27,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 29,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 35,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}},
              {LineColFunOp{.Line = 25,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 25,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 31,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 33,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_10_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context11_0) {
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 33,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 36,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 39,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 34,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 34,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 37,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 39,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_11_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context11_1) {
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 33,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 36,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 37,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 41,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 34,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 34,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 38,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 39,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_11_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context12_0) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
             LineColFunOp{.Line = 13,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},

           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
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
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_12_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context12_1) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 9,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_12_1_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context13_0) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
             LineColFunOp{.Line = 10,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 17,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 12,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 13,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 14,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 15,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_13_0_c_dbg.ll", GT, ContextAABuilder);
}

TEST(CtxSensUnionFindAATest, Context13_1) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 9,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_13_1_c_dbg.ll", GT, ContextAABuilder);
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

TEST(BotUnionFindAATest, Context04_0) {
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

  doAnalysisAndCompareResults("context_04_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context04_1) {
  GTMap GT = {{LineColFunOp{.Line = 7,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 7,
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
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_04_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context05_0) {
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

  doAnalysisAndCompareResults("context_05_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context05_1) {
  GTMap GT = {{LineColFunOp{.Line = 8,
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
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 16,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_05_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context06_0) {
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

  doAnalysisAndCompareResults("context_06_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context06_1) {
  GTMap GT = {{LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 10,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 14,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 17,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_06_1_c_dbg.ll", GT, BotAABuilder);
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

  doAnalysisAndCompareResults("context_07_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context08) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 19,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 21,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  doAnalysisAndCompareResults("context_08_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context09_0) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 16,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_09_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context09_1) {
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 17,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 20,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 22,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_09_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context10_0) {
  GTMap GT = {{LineColFunOp{.Line = 24,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 24,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 26,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 30,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 32,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_10_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context10_1) {
  GTMap GT = {
      {LineColFunOp{.Line = 24,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 24,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 27,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 29,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 35,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Load},
       }},
      {LineColFunOp{.Line = 25,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           LineColFunOp{.Line = 25,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
           LineColFunOp{.Line = 31,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
           LineColFunOp{.Line = 33,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Call},
       }},
  };

  doAnalysisAndCompareResults("context_10_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context11_0) {
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 33,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 36,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 39,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 39,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 34,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 34,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 37,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
            }}};

  doAnalysisAndCompareResults("context_11_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context11_1) {
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 33,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 36,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 37,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 41,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 34,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 34,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 38,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 39,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_11_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context12_0) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
             LineColFunOp{.Line = 13,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},

           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
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
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_12_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context12_1) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 9,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_12_1_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context13_0) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
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
             LineColFunOp{.Line = 10,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 17,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 12,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 13,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 14,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 15,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}}};

  doAnalysisAndCompareResults("context_13_0_c_dbg.ll", GT, BotAABuilder);
}

TEST(BotUnionFindAATest, Context13_1) {
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call}}},
           {LineColFunOp{.Line = 6,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 9,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}}};

  doAnalysisAndCompareResults("context_13_1_c_dbg.ll", GT, BotAABuilder);
}

// IndirectionSensUnionFindAA Tests

TEST(IndirectionSensUnionFindAATest, Basic01) {
  GTMap GT = {{LineColFunOp{.Line = 3,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 3,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};
  doAnalysisAndCompareResults("basic_01_c_dbg.ll", GT, IndAABuilder);

  psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
                                      "IndirectionSensUnionFindAA");
}

TEST(IndirectionSensUnionFindAATest, Basic02) {
  GTMap GT = {{
                  LineColFunOp{.Line = 3,
                               .Col = 0,
                               .InFunction = "main",
                               .OpCode = llvm::Instruction::Alloca},
                  {LineColFunOp{.Line = 3,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 6,
                                .Col = 4,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load}},
              },
              {
                  LineColFunOp{.Line = 4,
                               .Col = 0,
                               .InFunction = "main",
                               .OpCode = llvm::Instruction::Alloca},
                  {LineColFunOp{.Line = 4,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 6,
                                .Col = 5,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load}},
              },
              {
                  LineColFunOp{.Line = 4,
                               .Col = 0,
                               .InFunction = "main",
                               .OpCode = llvm::Instruction::Alloca},
                  {LineColFunOp{.Line = 4,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 6,
                                .Col = 5,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load}},
              }};

  doAnalysisAndCompareResults("basic_02_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Basic03) {
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

  doAnalysisAndCompareResults("basic_03_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Basic04) {
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

  doAnalysisAndCompareResults("basic_04_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context01) {
  /*
  ValueCompressor: {
    #0:
      %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
    #1:
      ptr %p | ID: id.0
    #2:
      %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
    #3:
      fun @id.<ret>
    #4:
      %retval = alloca i32, align 4, !psr.id !29 | ID: 5
    #5:
      %x = alloca i32, align 4, !psr.id !30 | ID: 6
    #6:
      %y = alloca i32, align 4, !psr.id !31 | ID: 7
    #7:
      %xx = alloca ptr, align 8, !psr.id !32 | ID: 8
    #8:
      %yy = alloca ptr, align 8, !psr.id !33 | ID: 9
    #9:
      %call = call ptr @id(ptr noundef %x), !dbg !46, !psr.id !47 | ID: 16
    #10:
      %call1 = call ptr @id(ptr noundef %y), !dbg !52, !psr.id !53 | ID: 19
    #11:
      %0 = load ptr, ptr %xx, align 8, !dbg !55, !psr.id !56 | ID: 21
  }
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 9, 10, 11>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <7>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <8>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
}
  */

  GTMap GT = {{LineColFunOp{.Line = 5,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {ArgInFun{.Idx = 0, .InFunction = "id"},
                LineColFunOp{.Line = 2,
                             .Col = 26,
                             .InFunction = "id",
                             .OpCode = llvm::Instruction::Load},
                RetVal{.InFunction = "id"},
                LineColFunOp{.Line = 5,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 6,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 8,
                             .Col = 13,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 9,
                             .Col = 13,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 11,
                             .Col = 11,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}},
              {LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};
  doAnalysisAndCompareResults("context_01_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context02) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    fun @id2.<ret>
  #8:
    %retval = alloca i32, align 4, !psr.id !39 | ID: 10
  #9:
    %x = alloca i32, align 4, !psr.id !40 | ID: 11
  #10:
    %y = alloca i32, align 4, !psr.id !41 | ID: 12
  #11:
    %xx1 = alloca ptr, align 8, !psr.id !42 | ID: 13
  #12:
    %xx2 = alloca ptr, align 8, !psr.id !43 | ID: 14
  #13:
    %yy1 = alloca ptr, align 8, !psr.id !44 | ID: 15
  #14:
    %yy2 = alloca ptr, align 8, !psr.id !45 | ID: 16
  #15:
    %call = call ptr @id1(ptr noundef %x), !dbg !58, !psr.id !59 | ID: 23
  #16:
    %call1 = call ptr @id1(ptr noundef %x), !dbg !64, !psr.id !65 | ID: 26
  #17:
    %call2 = call ptr @id2(ptr noundef %y), !dbg !70, !psr.id !71 | ID: 29
  #18:
    %call3 = call ptr @id2(ptr noundef %y), !dbg !76, !psr.id !77 | ID: 32
  #19:
    %0 = load ptr, ptr %xx1, align 8, !dbg !79, !psr.id !80 | ID: 34
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 9, 15, 16, 19>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <5, 6, 7, 10, 17, 18>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <8>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <11>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <12>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <13>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <14>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
}
  */
  GTMap GT = {{LineColFunOp{.Line = 6,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
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
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id2"},
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
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 11,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_02_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context03) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %retval = alloca i32, align 4, !psr.id !41 | ID: 11
  #10:
    %x = alloca i32, align 4, !psr.id !42 | ID: 12
  #11:
    %xx1 = alloca ptr, align 8, !psr.id !43 | ID: 13
  #12:
    %xx2 = alloca ptr, align 8, !psr.id !44 | ID: 14
  #13:
    %call = call ptr @id2(ptr noundef %x), !dbg !53, !psr.id !54 | ID: 19
  #14:
    %call1 = call ptr @id2(ptr noundef %x), !dbg !59, !psr.id !60 | ID: 22
  #15:
    %0 = load ptr, ptr %xx1, align 8, !dbg !62, !psr.id !63 | ID: 24
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 13, 14, 15>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <11>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <12>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
}
  */

  GTMap GT = {{LineColFunOp{.Line = 6,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
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
               }},
              {LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_03_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context04_0) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %retval = alloca i32, align 4, !psr.id !53 | ID: 17
  #15:
    %x = alloca i32, align 4, !psr.id !54 | ID: 18
  #16:
    %xx1 = alloca ptr, align 8, !psr.id !55 | ID: 19
  #17:
    %xx2 = alloca ptr, align 8, !psr.id !56 | ID: 20
  #18:
    %call = call ptr @id3(ptr noundef %x), !dbg !65, !psr.id !66 | ID: 25
  #19:
    %call1 = call ptr @id3(ptr noundef %x), !dbg !71, !psr.id !72 | ID: 28
  #20:
    %0 = load ptr, ptr %xx1, align 8, !dbg !74, !psr.id !75 | ID: 30
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 18, 19, 20>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <16>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <17>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
}

  */
  GTMap GT = {{LineColFunOp{.Line = 7,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
                   ArgInFun{.Idx = 0, .InFunction = "id3"},
                   LineColFunOp{.Line = 4,
                                .Col = 27,
                                .InFunction = "id3",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id3"},
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
               }},
              {LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_04_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context04_1) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %retval = alloca i32, align 4, !psr.id !53 | ID: 17
  #15:
    %x = alloca i32, align 4, !psr.id !54 | ID: 18
  #16:
    %y = alloca i32, align 4, !psr.id !55 | ID: 19
  #17:
    %xx1 = alloca ptr, align 8, !psr.id !56 | ID: 20
  #18:
    %xx2 = alloca ptr, align 8, !psr.id !57 | ID: 21
  #19:
    %yy1 = alloca ptr, align 8, !psr.id !58 | ID: 22
  #20:
    %yy2 = alloca ptr, align 8, !psr.id !59 | ID: 23
  #21:
    %call = call ptr @id3(ptr noundef %x), !dbg !72, !psr.id !73 | ID: 30
  #22:
    %call1 = call ptr @id3(ptr noundef %x), !dbg !78, !psr.id !79 | ID: 33
  #23:
    %call2 = call ptr @id3(ptr noundef %y), !dbg !84, !psr.id !85 | ID: 36
  #24:
    %call3 = call ptr @id3(ptr noundef %y), !dbg !90, !psr.id !91 | ID: 39
  #25:
    %0 = load ptr, ptr %xx1, align 8, !dbg !93, !psr.id !94 | ID: 41
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 21, 22, 23, 24, 25>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <17>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <18>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <19>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <20>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
}

  */
  GTMap GT = {{LineColFunOp{.Line = 7,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
                   ArgInFun{.Idx = 0, .InFunction = "id3"},
                   LineColFunOp{.Line = 4,
                                .Col = 27,
                                .InFunction = "id3",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id3"},
                   LineColFunOp{.Line = 7,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
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
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 13,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 15,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 11,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_04_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context05_0) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %s.addr = alloca ptr, align 8, !psr.id !51 | ID: 17
  #15:
    ptr %s | ID: id4.0
  #16:
    %0 = load ptr, ptr %s.addr, align 8, !dbg !56, !psr.id !57 | ID: 20
  #17:
    %call = call ptr @id3(ptr noundef %0), !dbg !58, !psr.id !59 | ID: 21
  #18:
    fun @id4.<ret>
  #19:
    %retval = alloca i32, align 4, !psr.id !65 | ID: 23
  #20:
    %x = alloca i32, align 4, !psr.id !66 | ID: 24
  #21:
    %xx = alloca ptr, align 8, !psr.id !67 | ID: 25
  #22:
    %yy = alloca ptr, align 8, !psr.id !68 | ID: 26
  #23:
    %call = call ptr @id4(ptr noundef %x), !dbg !77, !psr.id !78 | ID: 31
  #24:
    %call1 = call ptr @id4(ptr noundef %x), !dbg !83, !psr.id !84 | ID: 34
  #25:
    %0 = load ptr, ptr %xx, align 8, !dbg !86, !psr.id !87 | ID: 36
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 23, 24, 25>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <19>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <21>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <22>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
}

  */
  GTMap GT = {{LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
                   ArgInFun{.Idx = 0, .InFunction = "id3"},
                   LineColFunOp{.Line = 4,
                                .Col = 27,
                                .InFunction = "id3",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id3"},
                   ArgInFun{.Idx = 0, .InFunction = "id4"},
                   LineColFunOp{.Line = 5,
                                .Col = 27,
                                .InFunction = "id4",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id4"},
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
               }},
              {LineColFunOp{.Line = 10,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 11,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_05_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context05_1) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %s.addr = alloca ptr, align 8, !psr.id !51 | ID: 17
  #15:
    ptr %s | ID: id4.0
  #16:
    %0 = load ptr, ptr %s.addr, align 8, !dbg !56, !psr.id !57 | ID: 20
  #17:
    %call = call ptr @id3(ptr noundef %0), !dbg !58, !psr.id !59 | ID: 21
  #18:
    fun @id4.<ret>
  #19:
    %retval = alloca i32, align 4, !psr.id !65 | ID: 23
  #20:
    %x = alloca i32, align 4, !psr.id !66 | ID: 24
  #21:
    %y = alloca i32, align 4, !psr.id !67 | ID: 25
  #22:
    %xx1 = alloca ptr, align 8, !psr.id !68 | ID: 26
  #23:
    %xx2 = alloca ptr, align 8, !psr.id !69 | ID: 27
  #24:
    %yy1 = alloca ptr, align 8, !psr.id !70 | ID: 28
  #25:
    %yy2 = alloca ptr, align 8, !psr.id !71 | ID: 29
  #26:
    %call = call ptr @id4(ptr noundef %x), !dbg !84, !psr.id !85 | ID: 36
  #27:
    %call1 = call ptr @id4(ptr noundef %x), !dbg !90, !psr.id !91 | ID: 39
  #28:
    %call2 = call ptr @id4(ptr noundef %y), !dbg !96, !psr.id !97 | ID: 42
  #29:
    %call3 = call ptr @id4(ptr noundef %y), !dbg !102, !psr.id !103 | ID: 45
  #30:
    %0 = load ptr, ptr %xx2, align 8, !dbg !105, !psr.id !106 | ID: 47
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 26, 27, 28,
29, 30> #6: <> #7: <> #8: <> #9: <> #10: <4> #11: <> #12: <> #13: <> #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <19>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <22>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <23>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <24>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
  #45: <25>
  #46: <>
  #47: <>
  #48: <>
  #49: <>
}

  */
  GTMap GT = {{LineColFunOp{.Line = 8,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
                   ArgInFun{.Idx = 0, .InFunction = "id3"},
                   LineColFunOp{.Line = 4,
                                .Col = 27,
                                .InFunction = "id3",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id3"},
                   ArgInFun{.Idx = 0, .InFunction = "id4"},
                   LineColFunOp{.Line = 5,
                                .Col = 27,
                                .InFunction = "id4",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id4"},
                   LineColFunOp{.Line = 8,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
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
                   LineColFunOp{.Line = 13,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 14,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 16,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }},
              {LineColFunOp{.Line = 11,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 14,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_05_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context06_0) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %s.addr = alloca ptr, align 8, !psr.id !51 | ID: 17
  #15:
    ptr %s | ID: id4.0
  #16:
    %0 = load ptr, ptr %s.addr, align 8, !dbg !56, !psr.id !57 | ID: 20
  #17:
    %call = call ptr @id3(ptr noundef %0), !dbg !58, !psr.id !59 | ID: 21
  #18:
    fun @id4.<ret>
  #19:
    %t.addr = alloca ptr, align 8, !psr.id !63 | ID: 23
  #20:
    ptr %t | ID: id5.0
  #21:
    %0 = load ptr, ptr %t.addr, align 8, !dbg !68, !psr.id !69 | ID: 26
  #22:
    %call = call ptr @id4(ptr noundef %0), !dbg !70, !psr.id !71 | ID: 27
  #23:
    fun @id5.<ret>
  #24:
    %retval = alloca i32, align 4, !psr.id !77 | ID: 29
  #25:
    %x = alloca i32, align 4, !psr.id !78 | ID: 30
  #26:
    %xx = alloca ptr, align 8, !psr.id !79 | ID: 31
  #27:
    %yy = alloca ptr, align 8, !psr.id !80 | ID: 32
  #28:
    %call = call ptr @id5(ptr noundef %x), !dbg !89, !psr.id !90 | ID: 37
  #29:
    %call1 = call ptr @id5(ptr noundef %x), !dbg !95, !psr.id !96 | ID: 40
  #30:
    %0 = load ptr, ptr %xx, align 8, !dbg !98, !psr.id !99 | ID: 42
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 22, 23, 25,
28, 29, 30> #6: <> #7: <> #8: <> #9: <> #10: <4> #11: <> #12: <> #13: <> #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <19>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <24>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <26>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <27>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
}
  */
  GTMap GT = {{LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "id1"},
                   LineColFunOp{.Line = 2,
                                .Col = 27,
                                .InFunction = "id1",
                                .OpCode = llvm::Instruction::Load},
                   RetVal{.InFunction = "id1"},
                   ArgInFun{.Idx = 0, .InFunction = "id2"},
                   LineColFunOp{.Line = 3,
                                .Col = 27,
                                .InFunction = "id2",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id2"},
                   ArgInFun{.Idx = 0, .InFunction = "id3"},
                   LineColFunOp{.Line = 4,
                                .Col = 27,
                                .InFunction = "id3",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id3"},
                   ArgInFun{.Idx = 0, .InFunction = "id4"},
                   LineColFunOp{.Line = 5,
                                .Col = 27,
                                .InFunction = "id4",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id4"},
                   ArgInFun{.Idx = 0, .InFunction = "id5"},
                   LineColFunOp{.Line = 6,
                                .Col = 27,
                                .InFunction = "id5",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "id5"},
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
               }},
              {LineColFunOp{.Line = 11,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_06_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context06_1) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %p | ID: id1.0
  #2:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @id1.<ret>
  #4:
    %q.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %q | ID: id2.0
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @id1(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @id2.<ret>
  #9:
    %r.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %r | ID: id3.0
  #11:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @id2(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @id3.<ret>
  #14:
    %s.addr = alloca ptr, align 8, !psr.id !51 | ID: 17
  #15:
    ptr %s | ID: id4.0
  #16:
    %0 = load ptr, ptr %s.addr, align 8, !dbg !56, !psr.id !57 | ID: 20
  #17:
    %call = call ptr @id3(ptr noundef %0), !dbg !58, !psr.id !59 | ID: 21
  #18:
    fun @id4.<ret>
  #19:
    %t.addr = alloca ptr, align 8, !psr.id !63 | ID: 23
  #20:
    ptr %t | ID: id5.0
  #21:
    %0 = load ptr, ptr %t.addr, align 8, !dbg !68, !psr.id !69 | ID: 26
  #22:
    %call = call ptr @id4(ptr noundef %0), !dbg !70, !psr.id !71 | ID: 27
  #23:
    fun @id5.<ret>
  #24:
    %retval = alloca i32, align 4, !psr.id !77 | ID: 29
  #25:
    %x = alloca i32, align 4, !psr.id !78 | ID: 30
  #26:
    %y = alloca i32, align 4, !psr.id !79 | ID: 31
  #27:
    %xx1 = alloca ptr, align 8, !psr.id !80 | ID: 32
  #28:
    %xx2 = alloca ptr, align 8, !psr.id !81 | ID: 33
  #29:
    %yy1 = alloca ptr, align 8, !psr.id !82 | ID: 34
  #30:
    %yy2 = alloca ptr, align 8, !psr.id !83 | ID: 35
  #31:
    %call = call ptr @id5(ptr noundef %x), !dbg !96, !psr.id !97 | ID: 42
  #32:
    %call1 = call ptr @id5(ptr noundef %x), !dbg !102, !psr.id !103 | ID: 45
  #33:
    %call2 = call ptr @id5(ptr noundef %y), !dbg !108, !psr.id !109 | ID: 48
  #34:
    %call3 = call ptr @id5(ptr noundef %y), !dbg !114, !psr.id !115 | ID: 51
  #35:
    %0 = load ptr, ptr %yy1, align 8, !dbg !117, !psr.id !118 | ID: 53
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 22, 23, 25,
26, 31, 32, 33, 34, 35> #6: <> #7: <> #8: <> #9: <> #10: <4> #11: <> #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <19>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <24>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <27>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <28>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
  #45: <29>
  #46: <>
  #47: <>
  #48: <>
  #49: <>
  #50: <30>
  #51: <>
  #52: <>
  #53: <>
  #54: <>
}

  */
  GTMap GT = {{LineColFunOp{.Line = 9,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {ArgInFun{.Idx = 0, .InFunction = "id1"},
                LineColFunOp{.Line = 2,
                             .Col = 27,
                             .InFunction = "id1",
                             .OpCode = llvm::Instruction::Load},
                RetVal{.InFunction = "id1"},
                ArgInFun{.Idx = 0, .InFunction = "id2"},
                LineColFunOp{.Line = 3,
                             .Col = 27,
                             .InFunction = "id2",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "id2"},
                ArgInFun{.Idx = 0, .InFunction = "id3"},
                LineColFunOp{.Line = 4,
                             .Col = 27,
                             .InFunction = "id3",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "id3"},
                ArgInFun{.Idx = 0, .InFunction = "id4"},
                LineColFunOp{.Line = 5,
                             .Col = 27,
                             .InFunction = "id4",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "id4"},
                ArgInFun{.Idx = 0, .InFunction = "id5"},
                LineColFunOp{.Line = 6,
                             .Col = 27,
                             .InFunction = "id5",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "id5"},
                LineColFunOp{.Line = 9,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 17,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load}}},
              {LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 13,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 14,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}},
              {LineColFunOp{.Line = 15,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_06_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context07) {
  /*
  ValueCompressor: {
  #0:
    %s.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    ptr %s | ID: buzz.0
  #2:
    %0 = load ptr, ptr %s.addr, align 8, !dbg !22, !psr.id !23 | ID: 3
  #3:
    fun @buzz.<ret>
  #4:
    %r.addr = alloca ptr, align 8, !psr.id !27 | ID: 5
  #5:
    ptr %r | ID: baz.0
  #6:
    %0 = load ptr, ptr %r.addr, align 8, !dbg !32, !psr.id !33 | ID: 8
  #7:
    %call = call ptr @buzz(ptr noundef %0), !dbg !34, !psr.id !35 | ID: 9
  #8:
    fun @baz.<ret>
  #9:
    %q.addr = alloca ptr, align 8, !psr.id !39 | ID: 11
  #10:
    ptr %q | ID: bar.0
  #11:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
  #12:
    %call = call ptr @baz(ptr noundef %0), !dbg !46, !psr.id !47 | ID: 15
  #13:
    fun @bar.<ret>
  #14:
    %p.addr = alloca ptr, align 8, !psr.id !51 | ID: 17
  #15:
    ptr %p | ID: foo.0
  #16:
    %0 = load ptr, ptr %p.addr, align 8, !dbg !56, !psr.id !57 | ID: 20
  #17:
    %call = call ptr @bar(ptr noundef %0), !dbg !58, !psr.id !59 | ID: 21
  #18:
    fun @foo.<ret>
  #19:
    %retval = alloca i32, align 4, !psr.id !65 | ID: 23
  #20:
    %x = alloca i32, align 4, !psr.id !66 | ID: 24
  #21:
    %y = alloca i32, align 4, !psr.id !67 | ID: 25
  #22:
    %xx = alloca ptr, align 8, !psr.id !68 | ID: 26
  #23:
    %yy = alloca ptr, align 8, !psr.id !69 | ID: 27
  #24:
    %call = call ptr @foo(ptr noundef %x), !dbg !82, !psr.id !83 | ID: 34
  #25:
    %call1 = call ptr @foo(ptr noundef %y), !dbg !88, !psr.id !89 | ID: 37
  #26:
    %0 = load ptr, ptr %xx, align 8, !dbg !91, !psr.id !92 | ID: 39
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 24, 25, 26>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <4>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <14>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <19>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <22>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <23>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
}

  */
  GTMap GT = {
      {LineColFunOp{.Line = 8,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {
           ArgInFun{.Idx = 0, .InFunction = "buzz"},
           LineColFunOp{.Line = 2,
                        .Col = 28,
                        .InFunction = "buzz",
                        .OpCode = llvm::Instruction::Load},
           RetVal{.InFunction = "buzz"},
           ArgInFun{.Idx = 0, .InFunction = "baz"},
           LineColFunOp{.Line = 3,
                        .Col = 27,
                        .InFunction = "baz",
                        .OpCode = llvm::Instruction::Call},
           RetVal{.InFunction = "baz"},
           ArgInFun{.Idx = 0, .InFunction = "bar"},
           LineColFunOp{.Line = 4,
                        .Col = 27,
                        .InFunction = "bar",
                        .OpCode = llvm::Instruction::Call},
           RetVal{.InFunction = "bar"},
           ArgInFun{.Idx = 0, .InFunction = "foo"},
           LineColFunOp{.Line = 5,
                        .Col = 27,
                        .InFunction = "foo",
                        .OpCode = llvm::Instruction::Call},
           RetVal{.InFunction = "foo"},
           LineColFunOp{.Line = 8,
                        .Col = 0,
                        .InFunction = "main",
                        .OpCode = llvm::Instruction::Alloca},
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
       }},
      {LineColFunOp{.Line = 11,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {LineColFunOp{.Line = 11,
                     .Col = 0,
                     .InFunction = "main",
                     .OpCode = llvm::Instruction::Alloca}}},
      {LineColFunOp{.Line = 12,
                    .Col = 0,
                    .InFunction = "main",
                    .OpCode = llvm::Instruction::Alloca},
       {LineColFunOp{.Line = 12,
                     .Col = 0,
                     .InFunction = "main",
                     .OpCode = llvm::Instruction::Alloca}}},
  };

  doAnalysisAndCompareResults("context_07_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context08) {
  /*
  ValueCompressor: {
  #0:
    %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %Ptr | ID: selfRecursion.0
  #3:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
    %3 = load ptr, ptr %Ptr.addr, align 8, !dbg !38, !psr.id !39 | ID: 11
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !48, !psr.id !49 | ID: 16
  #4:
    %call = call ptr @selfRecursion(ptr noundef %6), !dbg !50, !psr.id !51 | ID:
17 #5: fun @selfRecursion.<ret> #6: %7 = load ptr, ptr %retval, align 8, !dbg
!55, !psr.id !56 | ID: 20 #7: %retval = alloca i32, align 4, !psr.id !61 | ID:
22 #8: %k = alloca i32, align 4, !psr.id !62 | ID: 23 #9: %kptr = alloca ptr,
align 8, !psr.id !63 | ID: 24 #10: %x = alloca ptr, align 8, !psr.id !64 | ID:
25 #11: %y = alloca ptr, align 8, !psr.id !65 | ID: 26 #12: %0 = load ptr, ptr
%kptr, align 8, !dbg !78, !psr.id !79 | ID: 33 %1 = load ptr, ptr %kptr, align
8, !dbg !88, !psr.id !89 | ID: 38 #13: %call = call ptr @selfRecursion(ptr
noundef %0), !dbg !80, !psr.id !81 | ID: 34 #14: %call1 = call ptr
@selfRecursion(ptr noundef %1), !dbg !90, !psr.id !91 | ID: 39 #15: %2 = load
ptr, ptr %x, align 8, !dbg !93, !psr.id !94 | ID: 41
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 6, 8, 12, 13, 14, 15>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <7>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <9>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <10>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <11>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
}

#10: <2, 3, 4, 5, 6, 8, 12, 13, 14, 15>

  */
  GTMap GT = {{LineColFunOp{.Line = 12,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Alloca},
               {
                   ArgInFun{.Idx = 0, .InFunction = "selfRecursion"},
                   LineColFunOp{.Line = 3,
                                .Col = 8,
                                .InFunction = "selfRecursion",
                                .OpCode = llvm::Instruction::Load},
                   LineColFunOp{.Line = 4,
                                .Col = 12,
                                .InFunction = "selfRecursion",
                                .OpCode = llvm::Instruction::Load},
                   LineColFunOp{.Line = 7,
                                .Col = 11,
                                .InFunction = "selfRecursion",
                                .OpCode = llvm::Instruction::Load},
                   LineColFunOp{.Line = 8,
                                .Col = 10,
                                .InFunction = "selfRecursion",
                                .OpCode = llvm::Instruction::Call},
                   RetVal{.InFunction = "selfRecursion"},
                   LineColFunOp{.Line = 12,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Alloca},
                   LineColFunOp{.Line = 15,
                                .Col = 12,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 19,
                                .Col = 12,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Call},
                   LineColFunOp{.Line = 21,
                                .Col = 0,
                                .InFunction = "main",
                                .OpCode = llvm::Instruction::Load},
               }}};

  psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
                                      "IndirectionSensUnionFindAA");

  doAnalysisAndCompareResults("context_08_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context09_0) {

  /*
ValueCompressor: {
  #0:
    %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %Ptr | ID: selfRecursion.0
  #3:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
    %3 = load ptr, ptr %Ptr.addr, align 8, !dbg !38, !psr.id !39 | ID: 11
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !48, !psr.id !49 | ID: 16
  #4:
    %call = call ptr @selfRecursion(ptr noundef %6), !dbg !50, !psr.id !51 | ID:
17 #5: fun @selfRecursion.<ret> #6: %7 = load ptr, ptr %retval, align 8, !dbg
!55, !psr.id !56 | ID: 20 #7: %retval = alloca i32, align 4, !psr.id !61 | ID:
22 #8: %k = alloca i32, align 4, !psr.id !62 | ID: 23 #9: %l = alloca i32, align
4, !psr.id !63 | ID: 24 #10: %x = alloca ptr, align 8, !psr.id !64 | ID: 25 #11:
    %y = alloca ptr, align 8, !psr.id !65 | ID: 26
  #12:
    %call = call ptr @selfRecursion(ptr noundef %k), !dbg !78, !psr.id !79 | ID:
33 #13: %call1 = call ptr @selfRecursion(ptr noundef %l), !dbg !84, !psr.id !85
| ID: 36 #14: %0 = load ptr, ptr %y, align 8, !dbg !87, !psr.id !88 | ID: 38
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 6, 8, 9, 12, 13, 14>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <7>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <10>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <11>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
}
  */

  // TODO: missing other entries. like x and y

  GTMap GT{{LineColFunOp{.Line = 12,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "selfRecursion"},
                LineColFunOp{.Line = 3,
                             .Col = 8,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 4,
                             .Col = 12,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 11,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 8,
                             .Col = 10,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "selfRecursion"},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 16,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 15,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 16,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 16,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_09_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context09_1) {

  /*
  ValueCompressor: {
    #0:
      %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
    #1:
      %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
    #2:
      ptr %Ptr | ID: selfRecursion.0
    #3:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
      %3 = load ptr, ptr %Ptr.addr, align 8, !dbg !38, !psr.id !39 | ID: 11
      %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !44, !psr.id !45 | ID: 14
      %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !48, !psr.id !49 | ID: 16
    #4:
      %call = call ptr @selfRecursion(ptr noundef %6), !dbg !50, !psr.id !51 |
  ID: 17 #5: fun @selfRecursion.<ret> #6: %7 = load ptr, ptr %retval, align 8,
  !dbg !55, !psr.id !56 | ID: 20 #7: %retval = alloca i32, align 4, !psr.id !61
  | ID: 22 #8: %k = alloca i32, align 4, !psr.id !62 | ID: 23 #9: %l = alloca
  i32, align 4, !psr.id !63 | ID: 24 #10: %xx1 = alloca ptr, align 8, !psr.id
  !64 | ID: 25 #11: %xx2 = alloca ptr, align 8, !psr.id !65 | ID: 26 #12: %yy1 =
  alloca ptr, align 8, !psr.id !66 | ID: 27 #13: %yy2 = alloca ptr, align 8,
  !psr.id !67 | ID: 28 #14: %call = call ptr @selfRecursion(ptr noundef %k),
  !dbg !80, !psr.id !81 | ID: 35 #15: %call1 = call ptr @selfRecursion(ptr
  noundef %k), !dbg !88, !psr.id !89 | ID: 39 #16: %call2 = call ptr
  @selfRecursion(ptr noundef %l), !dbg !94, !psr.id !95 | ID: 42 #17: %call3 =
  call ptr @selfRecursion(ptr noundef %l), !dbg !102, !psr.id !103 | ID: 46 #18:
      %0 = load ptr, ptr %yy2, align 8, !dbg !105, !psr.id !106 | ID: 48
  }
  UnionFindAAResult {
    #0: <0>
    #1: <>
    #2: <>
    #3: <>
    #4: <>
    #5: <1>
    #6: <>
    #7: <>
    #8: <>
    #9: <>
    #10: <2, 3, 4, 5, 6, 8, 9, 14, 15, 16, 17, 18>
    #11: <>
    #12: <>
    #13: <>
    #14: <>
    #15: <7>
    #16: <>
    #17: <>
    #18: <>
    #19: <>
    #20: <10>
    #21: <>
    #22: <>
    #23: <>
    #24: <>
    #25: <11>
    #26: <>
    #27: <>
    #28: <>
    #29: <>
    #30: <12>
    #31: <>
    #32: <>
    #33: <>
    #34: <>
    #35: <13>
    #36: <>
    #37: <>
    #38: <>
    #39: <>
  }
  */

  GTMap GT{{LineColFunOp{.Line = 12,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "selfRecursion"},
                LineColFunOp{.Line = 3,
                             .Col = 8,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 4,
                             .Col = 12,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 11,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 8,
                             .Col = 10,
                             .InFunction = "selfRecursion",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "selfRecursion"},
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 15,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 17,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 18,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 20,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 22,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 15,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 17,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 17,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 18,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 18,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 20,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 20,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_09_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context10_0) {
  /*
  ValueCompressor: {
  #0:
    %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %Ptr | ID: Forth.0
  #3:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
    %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !39, !psr.id !40 | ID: 11
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !43, !psr.id !44 | ID: 13
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !50, !psr.id !51 | ID: 17
  #4:
    %call = call ptr @Back(ptr noundef %5), !dbg !45, !psr.id !46 | ID: 14
  #5:
    fun @Back.<ret>
  #6:
    ptr %Ptr | ID: Back.0
  #7:
    %7 = load ptr, ptr %retval, align 8, !dbg !55, !psr.id !56 | ID: 20
  #8:
    fun @Forth.<ret>
  #9:
    %retval = alloca ptr, align 8, !psr.id !59 | ID: 22
  #10:
    %Ptr.addr = alloca ptr, align 8, !psr.id !60 | ID: 23
  #11:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !65, !psr.id !67 | ID: 26
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !74, !psr.id !76 | ID: 30
    %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !81, !psr.id !82 | ID: 33
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !85, !psr.id !86 | ID: 35
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !92, !psr.id !93 | ID: 39
  #12:
    %call = call ptr @Forth(ptr noundef %5), !dbg !87, !psr.id !88 | ID: 36
  #13:
    %7 = load ptr, ptr %retval, align 8, !dbg !97, !psr.id !98 | ID: 42
  #14:
    %retval = alloca i32, align 4, !psr.id !103 | ID: 44
  #15:
    %k = alloca i32, align 4, !psr.id !104 | ID: 45
  #16:
    %x = alloca ptr, align 8, !psr.id !105 | ID: 46
  #17:
    %y = alloca ptr, align 8, !psr.id !106 | ID: 47
  #18:
    %call = call ptr @Back(ptr noundef %k), !dbg !115, !psr.id !116 | ID: 52
  #19:
    %call1 = call ptr @Back(ptr noundef %k), !dbg !123, !psr.id !124 | ID: 56
  #20:
    %0 = load ptr, ptr %x, align 8, !dbg !126, !psr.id !127 | ID: 58
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 15, 18, 19, 20>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <9>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <10>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <14>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <16>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <17>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 24,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "Forth"},
                LineColFunOp{.Line = 5,
                             .Col = 8,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 6,
                             .Col = 13,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 12,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Forth"},
                ArgInFun{.Idx = 0, .InFunction = "Back"},
                LineColFunOp{.Line = 14,
                             .Col = 8,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 15,
                             .Col = 13,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 16,
                             .Col = 12,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Back"},
                LineColFunOp{.Line = 24,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 26,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 30,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 32,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 26,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 26,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 30,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 30,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_10_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context10_1) {
  /*
  ValueCompressor: {
    #0:
      %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
    #1:
      %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
    #2:
      ptr %Ptr | ID: Forth.0
    #3:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
      %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !39, !psr.id !40 | ID: 11
      %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !43, !psr.id !44 | ID: 13
      %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !50, !psr.id !51 | ID: 17
    #4:
      %call = call ptr @Back(ptr noundef %5), !dbg !45, !psr.id !46 | ID: 14
    #5:
      fun @Back.<ret>
    #6:
      ptr %Ptr | ID: Back.0
    #7:
      %7 = load ptr, ptr %retval, align 8, !dbg !55, !psr.id !56 | ID: 20
    #8:
      fun @Forth.<ret>
    #9:
      %retval = alloca ptr, align 8, !psr.id !59 | ID: 22
    #10:
      %Ptr.addr = alloca ptr, align 8, !psr.id !60 | ID: 23
    #11:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !65, !psr.id !67 | ID: 26
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !74, !psr.id !76 | ID: 30
      %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !81, !psr.id !82 | ID: 33
      %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !85, !psr.id !86 | ID: 35
      %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !92, !psr.id !93 | ID: 39
    #12:
      %call = call ptr @Forth(ptr noundef %5), !dbg !87, !psr.id !88 | ID: 36
    #13:
      %7 = load ptr, ptr %retval, align 8, !dbg !97, !psr.id !98 | ID: 42
    #14:
      %retval = alloca i32, align 4, !psr.id !103 | ID: 44
    #15:
      %k = alloca i32, align 4, !psr.id !104 | ID: 45
    #16:
      %l = alloca i32, align 4, !psr.id !105 | ID: 46
    #17:
      %xx1 = alloca ptr, align 8, !psr.id !106 | ID: 47
    #18:
      %xx2 = alloca ptr, align 8, !psr.id !107 | ID: 48
    #19:
      %yy1 = alloca ptr, align 8, !psr.id !108 | ID: 49
    #20:
      %yy2 = alloca ptr, align 8, !psr.id !109 | ID: 50
    #21:
      %call = call ptr @Back(ptr noundef %k), !dbg !122, !psr.id !123 | ID: 57
    #22:
      %call1 = call ptr @Back(ptr noundef %k), !dbg !130, !psr.id !131 | ID: 61
    #23:
      %call2 = call ptr @Back(ptr noundef %l), !dbg !136, !psr.id !137 | ID: 64
    #24:
      %call3 = call ptr @Back(ptr noundef %l), !dbg !144, !psr.id !145 | ID: 68
    #25:
      %0 = load ptr, ptr %xx1, align 8, !dbg !147, !psr.id !148 | ID: 70
  }
  UnionFindAAResult {
    #0: <0>
    #1: <>
    #2: <>
    #3: <>
    #4: <>
    #5: <1>
    #6: <>
    #7: <>
    #8: <>
    #9: <>
    #10: <2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 15, 16, 21, 22, 23, 24, 25>
    #11: <>
    #12: <>
    #13: <>
    #14: <>
    #15: <9>
    #16: <>
    #17: <>
    #18: <>
    #19: <>
    #20: <10>
    #21: <>
    #22: <>
    #23: <>
    #24: <>
    #25: <14>
    #26: <>
    #27: <>
    #28: <>
    #29: <>
    #30: <17>
    #31: <>
    #32: <>
    #33: <>
    #34: <>
    #35: <18>
    #36: <>
    #37: <>
    #38: <>
    #39: <>
    #40: <19>
    #41: <>
    #42: <>
    #43: <>
    #44: <>
    #45: <20>
    #46: <>
    #47: <>
    #48: <>
    #49: <>
  }
  */
  GTMap GT{{LineColFunOp{.Line = 24,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "Forth"},
                LineColFunOp{.Line = 5,
                             .Col = 8,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 6,
                             .Col = 13,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 12,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Forth"},
                ArgInFun{.Idx = 0, .InFunction = "Back"},
                LineColFunOp{.Line = 14,
                             .Col = 8,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 15,
                             .Col = 13,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 16,
                             .Col = 12,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Back"},
                LineColFunOp{.Line = 24,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 25,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 27,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 29,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 31,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 33,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 35,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 27,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 27,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 29,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 29,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 31,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 31,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 33,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_10_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context11_0) {
  /*
  ValueCompressor: {
    #0:
      %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
    #1:
      %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
    #2:
      ptr %Ptr | ID: Forth.0
    #3:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
      %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !39, !psr.id !40 | ID: 11
      %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !43, !psr.id !44 | ID: 13
      %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !50, !psr.id !51 | ID: 17
    #4:
      %call = call ptr @Stop(ptr noundef %5), !dbg !45, !psr.id !46 | ID: 14
    #5:
      fun @Stop.<ret>
    #6:
      ptr %Ptr | ID: Stop.0
    #7:
      %7 = load ptr, ptr %retval, align 8, !dbg !55, !psr.id !56 | ID: 20
    #8:
      fun @Forth.<ret>
    #9:
      %retval = alloca ptr, align 8, !psr.id !59 | ID: 22
    #10:
      %Ptr.addr = alloca ptr, align 8, !psr.id !60 | ID: 23
    #11:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !65, !psr.id !67 | ID: 26
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !76, !psr.id !78 | ID: 31
      %3 = load ptr, ptr %Ptr.addr, align 8, !dbg !84, !psr.id !85 | ID: 35
    #12:
      %call = call ptr @Forth(ptr noundef %2), !dbg !79, !psr.id !80 | ID: 32
    #13:
      %call1 = call ptr @Back(ptr noundef %3), !dbg !86, !psr.id !87 | ID: 36
    #14:
      fun @Back.<ret>
    #15:
      ptr %Ptr | ID: Back.0
    #16:
      %4 = load ptr, ptr %retval, align 8, !dbg !91, !psr.id !92 | ID: 39
    #17:
      %retval = alloca ptr, align 8, !psr.id !95 | ID: 41
    #18:
      %Ptr.addr = alloca ptr, align 8, !psr.id !96 | ID: 42
    #19:
      %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !101, !psr.id !103 | ID: 45
      %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !110, !psr.id !112 | ID: 49
      %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !117, !psr.id !118 | ID: 52
      %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !121, !psr.id !122 | ID: 54
      %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !128, !psr.id !129 | ID: 58
    #20:
      %call = call ptr @Stop(ptr noundef %5), !dbg !123, !psr.id !124 | ID: 55
    #21:
      %7 = load ptr, ptr %retval, align 8, !dbg !133, !psr.id !134 | ID: 61
    #22:
      %retval = alloca i32, align 4, !psr.id !139 | ID: 63
    #23:
      %k = alloca i32, align 4, !psr.id !140 | ID: 64
    #24:
      %l = alloca i32, align 4, !psr.id !141 | ID: 65
    #25:
      %x = alloca ptr, align 8, !psr.id !142 | ID: 66
    #26:
      %y = alloca ptr, align 8, !psr.id !143 | ID: 67
    #27:
      %call = call ptr @Back(ptr noundef %k), !dbg !156, !psr.id !157 | ID: 74
    #28:
      %call1 = call ptr @Forth(ptr noundef %l), !dbg !162, !psr.id !163 | ID: 77
    #29:
      %0 = load ptr, ptr %x, align 8, !dbg !165, !psr.id !166 | ID: 79
  }
  UnionFindAAResult {
    #0: <0>
    #1: <>
    #2: <>
    #3: <>
    #4: <>
    #5: <1>
    #6: <>
    #7: <>
    #8: <>
    #9: <>
    #10: <2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 19, 20, 21, 23, 24, 27,
  28, 29> #11: <> #12: <> #13: <> #14: <> #15: <9> #16: <> #17: <> #18: <> #19:
  <> #20: <10> #21: <> #22: <> #23: <> #24: <> #25: <17> #26: <> #27: <> #28: <>
    #29: <>
    #30: <18>
    #31: <>
    #32: <>
    #33: <>
    #34: <>
    #35: <22>
    #36: <>
    #37: <>
    #38: <>
    #39: <>
    #40: <25>
    #41: <>
    #42: <>
    #43: <>
    #44: <>
    #45: <26>
    #46: <>
    #47: <>
    #48: <>
    #49: <>
  }
  */
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "Forth"},
                LineColFunOp{.Line = 6,
                             .Col = 8,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 13,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 8,
                             .Col = 12,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Forth"},
                ArgInFun{.Idx = 0, .InFunction = "Back"},
                LineColFunOp{.Line = 15,
                             .Col = 8,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 16,
                             .Col = 13,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 17,
                             .Col = 12,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Back"},
                ArgInFun{.Idx = 0, .InFunction = "Stop"},
                LineColFunOp{.Line = 24,
                             .Col = 8,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 25,
                             .Col = 12,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 28,
                             .Col = 10,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Stop"},
                LineColFunOp{.Line = 33,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 34,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 36,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 37,
                             .Col = 12,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 39,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 36,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 36,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 37,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 37,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_11_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context11_1) {
  /*
  ValueCompressor: {
  #0:
    %retval = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %Ptr.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %Ptr | ID: Forth.0
  #3:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !23, !psr.id !25 | ID: 4
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !32, !psr.id !34 | ID: 8
    %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !39, !psr.id !40 | ID: 11
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !43, !psr.id !44 | ID: 13
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !50, !psr.id !51 | ID: 17
  #4:
    %call = call ptr @Stop(ptr noundef %5), !dbg !45, !psr.id !46 | ID: 14
  #5:
    fun @Stop.<ret>
  #6:
    ptr %Ptr | ID: Stop.0
  #7:
    %7 = load ptr, ptr %retval, align 8, !dbg !55, !psr.id !56 | ID: 20
  #8:
    fun @Forth.<ret>
  #9:
    %retval = alloca ptr, align 8, !psr.id !59 | ID: 22
  #10:
    %Ptr.addr = alloca ptr, align 8, !psr.id !60 | ID: 23
  #11:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !65, !psr.id !67 | ID: 26
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !76, !psr.id !78 | ID: 31
    %3 = load ptr, ptr %Ptr.addr, align 8, !dbg !84, !psr.id !85 | ID: 35
  #12:
    %call = call ptr @Forth(ptr noundef %2), !dbg !79, !psr.id !80 | ID: 32
  #13:
    %call1 = call ptr @Back(ptr noundef %3), !dbg !86, !psr.id !87 | ID: 36
  #14:
    fun @Back.<ret>
  #15:
    ptr %Ptr | ID: Back.0
  #16:
    %4 = load ptr, ptr %retval, align 8, !dbg !91, !psr.id !92 | ID: 39
  #17:
    %retval = alloca ptr, align 8, !psr.id !95 | ID: 41
  #18:
    %Ptr.addr = alloca ptr, align 8, !psr.id !96 | ID: 42
  #19:
    %0 = load ptr, ptr %Ptr.addr, align 8, !dbg !101, !psr.id !103 | ID: 45
    %2 = load ptr, ptr %Ptr.addr, align 8, !dbg !110, !psr.id !112 | ID: 49
    %4 = load ptr, ptr %Ptr.addr, align 8, !dbg !117, !psr.id !118 | ID: 52
    %5 = load ptr, ptr %Ptr.addr, align 8, !dbg !121, !psr.id !122 | ID: 54
    %6 = load ptr, ptr %Ptr.addr, align 8, !dbg !128, !psr.id !129 | ID: 58
  #20:
    %call = call ptr @Stop(ptr noundef %5), !dbg !123, !psr.id !124 | ID: 55
  #21:
    %7 = load ptr, ptr %retval, align 8, !dbg !133, !psr.id !134 | ID: 61
  #22:
    %retval = alloca i32, align 4, !psr.id !139 | ID: 63
  #23:
    %k = alloca i32, align 4, !psr.id !140 | ID: 64
  #24:
    %l = alloca i32, align 4, !psr.id !141 | ID: 65
  #25:
    %xx1 = alloca ptr, align 8, !psr.id !142 | ID: 66
  #26:
    %xx2 = alloca ptr, align 8, !psr.id !143 | ID: 67
  #27:
    %yy1 = alloca ptr, align 8, !psr.id !144 | ID: 68
  #28:
    %yy2 = alloca ptr, align 8, !psr.id !145 | ID: 69
  #29:
    %call = call ptr @Back(ptr noundef %k), !dbg !158, !psr.id !159 | ID: 76
  #30:
    %call1 = call ptr @Back(ptr noundef %k), !dbg !164, !psr.id !165 | ID: 79
  #31:
    %call2 = call ptr @Forth(ptr noundef %l), !dbg !170, !psr.id !171 | ID: 82
  #32:
    %call3 = call ptr @Forth(ptr noundef %l), !dbg !176, !psr.id !177 | ID: 85
  #33:
    %0 = load ptr, ptr %xx2, align 8, !dbg !179, !psr.id !180 | ID: 87
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 19, 20, 21, 23, 24, 29, 30,
31, 32, 33> #11: <> #12: <> #13: <> #14: <> #15: <9> #16: <> #17: <> #18: <>
  #19: <>
  #20: <10>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <17>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <18>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <22>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <25>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
  #45: <26>
  #46: <>
  #47: <>
  #48: <>
  #49: <>
  #50: <27>
  #51: <>
  #52: <>
  #53: <>
  #54: <>
  #55: <28>
  #56: <>
  #57: <>
  #58: <>
  #59: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 33,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                ArgInFun{.Idx = 0, .InFunction = "Forth"},
                LineColFunOp{.Line = 6,
                             .Col = 8,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 7,
                             .Col = 13,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 8,
                             .Col = 12,
                             .InFunction = "Forth",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Forth"},
                ArgInFun{.Idx = 0, .InFunction = "Back"},
                LineColFunOp{.Line = 15,
                             .Col = 8,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 16,
                             .Col = 13,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 17,
                             .Col = 12,
                             .InFunction = "Back",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Back"},
                ArgInFun{.Idx = 0, .InFunction = "Stop"},
                LineColFunOp{.Line = 24,
                             .Col = 8,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Load},
                LineColFunOp{.Line = 25,
                             .Col = 12,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 28,
                             .Col = 10,
                             .InFunction = "Stop",
                             .OpCode = llvm::Instruction::Call},
                RetVal{.InFunction = "Stop"},
                LineColFunOp{.Line = 33,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 34,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
                LineColFunOp{.Line = 36,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 37,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 38,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 39,
                             .Col = 14,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Call},
                LineColFunOp{.Line = 41,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Load},
            }},
           {LineColFunOp{.Line = 36,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 36,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 37,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 37,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 38,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 38,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 39,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 39,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_11_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context12_0) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %q.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %p | ID: argretq.0
  #3:
    ptr %q | ID: argretq.1
  #4:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !27, !psr.id !28 | ID: 6
  #5:
    fun @argretq.<ret>
  #6:
    %retval = alloca i32, align 4, !psr.id !34 | ID: 8
  #7:
    %x = alloca i32, align 4, !psr.id !35 | ID: 9
  #8:
    %y = alloca i32, align 4, !psr.id !36 | ID: 10
  #9:
    %xx1 = alloca ptr, align 8, !psr.id !37 | ID: 11
  #10:
    %xx2 = alloca ptr, align 8, !psr.id !38 | ID: 12
  #11:
    %yy1 = alloca ptr, align 8, !psr.id !39 | ID: 13
  #12:
    %yy2 = alloca ptr, align 8, !psr.id !40 | ID: 14
  #13:
    %call = call ptr @argretq(ptr noundef %y, ptr noundef %x), !dbg !53, !psr.id
!54 | ID: 21 #14: %call1 = call ptr @argretq(ptr noundef %y, ptr noundef %x),
!dbg !59, !psr.id !60 | ID: 24 #15: %call2 = call ptr @argretq(ptr noundef %x,
ptr noundef %y), !dbg !65, !psr.id !66 | ID: 27 #16: %call3 = call ptr
@argretq(ptr noundef %x, ptr noundef %y), !dbg !71, !psr.id !72 | ID: 30 #17: %0
= load ptr, ptr %xx1, align 8, !dbg !74, !psr.id !75 | ID: 32
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 7, 8, 13, 14, 15, 16, 17>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <6>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <9>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <10>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <11>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <12>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {ArgInFun{.Idx = 0, .InFunction = "argretq"},
             ArgInFun{.Idx = 1, .InFunction = "argretq"},
             RetVal{.InFunction = "argretq"},
             LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 9,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 10,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 13,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 8,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
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
            }},
           {LineColFunOp{.Line = 10,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 11,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_12_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context12_1) {
  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %q.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    ptr %p | ID: argretq.0
  #3:
    ptr %q | ID: argretq.1
  #4:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !27, !psr.id !28 | ID: 6
  #5:
    fun @argretq.<ret>
  #6:
    %retval = alloca i32, align 4, !psr.id !34 | ID: 8
  #7:
    %x = alloca i32, align 4, !psr.id !35 | ID: 9
  #8:
    %y = alloca i32, align 4, !psr.id !36 | ID: 10
  #9:
    %xx1 = alloca ptr, align 8, !psr.id !37 | ID: 11
  #10:
    %yy1 = alloca ptr, align 8, !psr.id !38 | ID: 12
  #11:
    %call = call ptr @argretq(ptr noundef %y, ptr noundef %x), !dbg !51, !psr.id
!52 | ID: 19 #12: %call1 = call ptr @argretq(ptr noundef %x, ptr noundef %y),
!dbg !57, !psr.id !58 | ID: 22 #13: %0 = load ptr, ptr %xx1, align 8, !dbg !60,
!psr.id !61 | ID: 24
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2, 3, 4, 5, 7, 8, 11, 12, 13>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <6>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <9>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <10>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {ArgInFun{.Idx = 0, .InFunction = "argretq"},
             ArgInFun{.Idx = 1, .InFunction = "argretq"},
             RetVal{.InFunction = "argretq"},
             LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 9,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 8,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
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
            }}};

  doAnalysisAndCompareResults("context_12_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context13_0) {

  /*
  ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %q.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    %r.addr = alloca ptr, align 8, !psr.id !19 | ID: 2
  #3:
    ptr %p | ID: argretq.0
  #4:
    ptr %q | ID: argretq.1
  #5:
    ptr %r | ID: argretq.2
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 9
  #7:
    fun @argretq.<ret>
  #8:
    %retval = alloca i32, align 4, !psr.id !39 | ID: 11
  #9:
    %x = alloca i32, align 4, !psr.id !40 | ID: 12
  #10:
    %y = alloca i32, align 4, !psr.id !41 | ID: 13
  #11:
    %xx1 = alloca ptr, align 8, !psr.id !42 | ID: 14
  #12:
    %xx2 = alloca ptr, align 8, !psr.id !43 | ID: 15
  #13:
    %xx3 = alloca ptr, align 8, !psr.id !44 | ID: 16
  #14:
    %xx4 = alloca ptr, align 8, !psr.id !45 | ID: 17
  #15:
    %yy1 = alloca ptr, align 8, !psr.id !46 | ID: 18
  #16:
    %yy2 = alloca ptr, align 8, !psr.id !47 | ID: 19
  #17:
    %yy3 = alloca ptr, align 8, !psr.id !48 | ID: 20
  #18:
    %yy4 = alloca ptr, align 8, !psr.id !49 | ID: 21
  #19:
    %call = call ptr @argretq(ptr noundef %x, ptr noundef %x, ptr noundef %x),
!dbg !62, !psr.id !63 | ID: 28 #20: %call1 = call ptr @argretq(ptr noundef %x,
ptr noundef %x, ptr noundef %y), !dbg !68, !psr.id !69 | ID: 31 #21: %call2 =
call ptr @argretq(ptr noundef %y, ptr noundef %x, ptr noundef %y), !dbg !74,
!psr.id !75 | ID: 34 #22: %call3 = call ptr @argretq(ptr noundef %y, ptr noundef
%x, ptr noundef %x), !dbg !80, !psr.id !81 | ID: 37 #23: %call4 = call ptr
@argretq(ptr noundef %x, ptr noundef %y, ptr noundef %x), !dbg !86, !psr.id !87
| ID: 40 #24: %call5 = call ptr @argretq(ptr noundef %x, ptr noundef %y, ptr
noundef %y), !dbg !92, !psr.id !93 | ID: 43 #25: %call6 = call ptr @argretq(ptr
noundef %y, ptr noundef %y, ptr noundef %y), !dbg !98, !psr.id !99 | ID: 46 #26:
    %call7 = call ptr @argretq(ptr noundef %y, ptr noundef %y, ptr noundef %x),
!dbg !104, !psr.id !105 | ID: 49 #27: %0 = load ptr, ptr %xx1, align 8, !dbg
!107, !psr.id !108 | ID: 51
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <3, 4, 5, 6, 7, 9, 10, 19, 20, 21, 22, 23, 24, 25, 26, 27>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <8>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <11>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <12>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
  #35: <13>
  #36: <>
  #37: <>
  #38: <>
  #39: <>
  #40: <14>
  #41: <>
  #42: <>
  #43: <>
  #44: <>
  #45: <15>
  #46: <>
  #47: <>
  #48: <>
  #49: <>
  #50: <16>
  #51: <>
  #52: <>
  #53: <>
  #54: <>
  #55: <17>
  #56: <>
  #57: <>
  #58: <>
  #59: <>
  #60: <18>
  #61: <>
  #62: <>
  #63: <>
  #64: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {ArgInFun{.Idx = 0, .InFunction = "argretq"},
             ArgInFun{.Idx = 1, .InFunction = "argretq"},
             ArgInFun{.Idx = 2, .InFunction = "argretq"},
             RetVal{.InFunction = "argretq"},
             LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 9,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 10,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 12,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 13,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 14,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 15,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 17,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 8,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
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
            }},
           {LineColFunOp{.Line = 10,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 10,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 11,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 11,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 12,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 12,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 13,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 13,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 14,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 14,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }},
           {LineColFunOp{.Line = 15,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 15,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
            }}};

  doAnalysisAndCompareResults("context_13_0_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context13_1) {
  /*
ValueCompressor: {
  #0:
    %p.addr = alloca ptr, align 8, !psr.id !17 | ID: 0
  #1:
    %q.addr = alloca ptr, align 8, !psr.id !18 | ID: 1
  #2:
    %r.addr = alloca ptr, align 8, !psr.id !19 | ID: 2
  #3:
    ptr %p | ID: argretq.0
  #4:
    ptr %q | ID: argretq.1
  #5:
    ptr %r | ID: argretq.2
  #6:
    %0 = load ptr, ptr %q.addr, align 8, !dbg !32, !psr.id !33 | ID: 9
  #7:
    fun @argretq.<ret>
  #8:
    %retval = alloca i32, align 4, !psr.id !39 | ID: 11
  #9:
    %x = alloca i32, align 4, !psr.id !40 | ID: 12
  #10:
    %y = alloca i32, align 4, !psr.id !41 | ID: 13
  #11:
    %xx1 = alloca ptr, align 8, !psr.id !42 | ID: 14
  #12:
    %yy1 = alloca ptr, align 8, !psr.id !43 | ID: 15
  #13:
    %call = call ptr @argretq(ptr noundef %x, ptr noundef %x, ptr noundef %x),
!dbg !56, !psr.id !57 | ID: 22 #14: %call1 = call ptr @argretq(ptr noundef %y,
ptr noundef %y, ptr noundef %y), !dbg !62, !psr.id !63 | ID: 25 #15: %0 = load
ptr, ptr %yy1, align 8, !dbg !65, !psr.id !66 | ID: 27
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <2>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <3, 4, 5, 6, 7, 9, 10, 13, 14, 15>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <8>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
  #25: <11>
  #26: <>
  #27: <>
  #28: <>
  #29: <>
  #30: <12>
  #31: <>
  #32: <>
  #33: <>
  #34: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 5,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {ArgInFun{.Idx = 0, .InFunction = "argretq"},
             ArgInFun{.Idx = 1, .InFunction = "argretq"},
             ArgInFun{.Idx = 2, .InFunction = "argretq"},
             RetVal{.InFunction = "argretq"},
             LineColFunOp{.Line = 5,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 8,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 9,
                          .Col = 14,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Call},
             LineColFunOp{.Line = 11,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Load}}},
           {LineColFunOp{.Line = 8,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {
                LineColFunOp{.Line = 8,
                             .Col = 0,
                             .InFunction = "main",
                             .OpCode = llvm::Instruction::Alloca},
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
            }}};

  doAnalysisAndCompareResults("context_13_1_c_dbg.ll", GT, IndAABuilder);
}

TEST(IndirectionSensUnionFindAATest, Context14_0) {
  /*
  ValueCompressor: {
  #0:
    %Func.addr = alloca ptr, align 8, !psr.id !24 | ID: 1
  #1:
    ptr %Func | ID: callback.0
  #2:
    %0 = load ptr, ptr %Func.addr, align 8, !dbg !29, !psr.id !30 | ID: 4
  #3:
    %retval = alloca i32, align 4, !psr.id !36 | ID: 7
  #4:
    %FuncPtr = alloca ptr, align 8, !psr.id !37 | ID: 8
  #5:
    %Zero = alloca i32, align 4, !psr.id !38 | ID: 9
  #6:
    fun @ret0
  #7:
    %0 = load ptr, ptr %FuncPtr, align 8, !dbg !47, !psr.id !48 | ID: 14
}
UnionFindAAResult {
  #0: <0>
  #1: <>
  #2: <>
  #3: <>
  #4: <>
  #5: <1, 2, 6, 7>
  #6: <>
  #7: <>
  #8: <>
  #9: <>
  #10: <3>
  #11: <>
  #12: <>
  #13: <>
  #14: <>
  #15: <4>
  #16: <>
  #17: <>
  #18: <>
  #19: <>
  #20: <5>
  #21: <>
  #22: <>
  #23: <>
  #24: <>
}
  */
  GTMap GT{{LineColFunOp{.Line = 7,
                         .Col = 0,
                         .InFunction = "main",
                         .OpCode = llvm::Instruction::Alloca},
            {LineColFunOp{.Line = 7,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca},
             LineColFunOp{.Line = 6,
                          .Col = 0,
                          .InFunction = "main",
                          .OpCode = llvm::Instruction::Alloca}}}};

  doAnalysisAndCompareResults("context_14_0_c_dbg.ll", GT, IndAABuilder);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
