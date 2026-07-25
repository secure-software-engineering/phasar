#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <map>
#include <memory>
#include <source_location>
#include <vector>

namespace {
using namespace psr;
using namespace psr::unittest;

static_assert(UnionFindAAResult<AndersenOTFResult>);

constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("pointers/");

using TSL = TestingSrcLocation;
using GTMap = std::map<TSL, std::vector<TSL>>;

[[nodiscard]] ValueId asId(const ValueCompressor<PAGVariable> &Compressor,
                           const LLVMProjectIRDB &IRDB, TSL Var) {
  const auto *LLVMVar = testingLocInIR(Var, IRDB);
  auto MaybeId = Compressor.getOrNull(LLVMVar);
  if (!MaybeId) {
    ADD_FAILURE() << "Value not in VC: " << Var;
    return ValueId{};
  }
  return *MaybeId;
}

[[nodiscard]] std::string
stringifyVal(const ValueCompressor<PAGVariable> &Compressor, ValueId VId) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);
  ROS << "{ ";
  llvm::interleaveComma(Compressor.id2vars(VId), ROS,
                        [&](PAGVariable Var) { ROS << to_string(Var); });
  ROS << " }";
  return Ret;
}

void dumpAnalysisState(const ValueCompressor<PAGVariable> &Compressor,
                       const AndersenOTFResult &Results) {
  llvm::errs() << "ValueCompressor: {\n";
  for (const auto &[VId, Values] : Compressor.id2vars().enumerate()) {
    llvm::errs() << "  #" << uint32_t(VId) << ":\n";
    for (const auto Val : Values) {
      llvm::errs() << "    " << to_string(Val) << '\n';
    }
  }
  llvm::errs() << "}\n";
  llvm::errs() << "AliasSets: {\n";
  for (const auto &[VId, Aliases] : Results.AliasSets.enumerate()) {
    bool First = true;
    for (const auto &Var : Compressor.id2vars(VId)) {
      llvm::errs() << "  " << to_string(Var);

      if (First) {
        First = false;
      } else {
        llvm::errs() << "  MUST ALIAS with "
                     << to_string(*Compressor.id2vars(VId).begin()) << '\n';
        continue;
      }

      if (Aliases.empty()) {
        llvm::errs() << " aliases: EMPTY\n";
        continue;
      }

      llvm::errs() << " aliases: {\n";
      Aliases.foreach ([&](ValueId AId) {
        llvm::errs() << "    " << stringifyVal(Compressor, AId) << '\n';
      });
      llvm::errs() << "  }\n";
    }
  }
  llvm::errs() << "}\n";
}

constexpr llvm::StringRef EntryNames[] = {"main"};

/// Exact bidirectional GT check.
///
/// Soundness: every alias listed in the GT must appear in the computed set.
/// Precision: no computed alias that is named in the GT (the "domain") may
/// be absent from the expected set.  Values not named in the GT are outside
/// the domain and are not subject to the precision check.
void doAnalysisAndCheckExact(
    const llvm::Twine &IRFile, const GTMap &ExpectedResults,
    bool DumpResults = false,
    std::source_location Loc = std::source_location::current()) {

  auto IRDB = LLVMProjectIRDB::loadOrExit(PathToLLFiles + IRFile);

  llvm::SmallVector<const llvm::Function *, 4> Entries;
  for (llvm::StringRef Name : EntryNames) {
    const auto *Func = IRDB.getFunctionDefinition(Name);
    if (!Func) {
      ADD_FAILURE_AT(Loc.file_name(), Loc.line())
          << "Entry function not found: " << Name.str();
      return;
    }
    Entries.push_back(Func);
  }

  ValueCompressor<PAGVariable> Compressor;
  AndersenOTFResult Results = computeAndersenOTFRaw(IRDB, Entries, &Compressor);

  // Build domain from all values explicitly named in the GT.
  llvm::SmallDenseSet<ValueId, 16> Domain;
  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    Domain.insert(asId(Compressor, IRDB, PtrVar));
    for (const auto &AliasVar : ExpectedAliasVars) {
      Domain.insert(asId(Compressor, IRDB, AliasVar));
    }
  }

  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    const auto PtrId = asId(Compressor, IRDB, PtrVar);
    const auto &Computed = Results.getRawAliasSet(PtrId);

    RawAliasSet<ValueId> Expected;
    // llvm::errs() << "For PtrId: #" << uint32_t(PtrId) << ":\n";
    for (const auto &AliasVar : ExpectedAliasVars) {
      auto AliasId = asId(Compressor, IRDB, AliasVar);
      Expected.insert(AliasId);
      // llvm::errs() << "> Insert #" << uint32_t(AliasId)
      //              << " into Expected due to " << AliasVar << '\n';
    }

    // Soundness.
    Expected.foreach ([&](ValueId AliasId) {
      if (!Computed.contains(AliasId)) {
        ADD_FAILURE_AT(Loc.file_name(), Loc.line())
            << "Missing expected alias of " << PtrVar << "(#" << uint32_t(PtrId)
            << "): #" << uint32_t(AliasId) << " as "
            << stringifyVal(Compressor, AliasId);
      }
    });

    // Precision (domain-restricted).
    Computed.foreach ([&](ValueId VId) {
      if (!Domain.contains(VId) || Expected.contains(VId)) {
        return;
      }
      ADD_FAILURE_AT(Loc.file_name(), Loc.line())
          << "Unexpected alias of " << PtrVar << ": "
          << stringifyVal(Compressor, VId);
    });
  }

  if (DumpResults || ::testing::Test::HasFailure()) {
    dumpAnalysisState(Compressor, Results);
  }
}

// ---- Tests ----------------------------------------------------------------

TEST(AndersenOTFAATest, InterProcArgRetAlias) {
  // retptr(x) returns x — formal parameter and return value must alias.
  const GTMap ExpectedResults = {
      {TSL(ArgInFun{.Idx = 0, .InFunction = "retptr"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "retptr"}),
        TSL(RetVal{.InFunction = "retptr"})}},
      {TSL(RetVal{.InFunction = "retptr"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "retptr"}),
        TSL(RetVal{.InFunction = "retptr"})}},
  };
  doAnalysisAndCheckExact("andersen_otf_interproc_c_m2r_dbg.ll",
                          ExpectedResults);
}

TEST(AndersenOTFAATest, FuncPtrArgRetAlias) {
  // id(x) returns x, called only via function pointer.
  // OTF must discover id as a callee and propagate arg/ret alias.
  const GTMap ExpectedResults = {
      {TSL(ArgInFun{.Idx = 0, .InFunction = "id"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id"}),
        TSL(RetVal{.InFunction = "id"})}},
      {TSL(RetVal{.InFunction = "id"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id"}),
        TSL(RetVal{.InFunction = "id"})}},
  };
  doAnalysisAndCheckExact("andersen_otf_fp_c_m2r_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FuncByNameInVC) {
  // The function 'id' has its address stored into fp; it must appear in VC.
  auto IRDB = LLVMProjectIRDB::loadOrExit(
      PathToLLFiles + llvm::Twine("andersen_otf_fp_c_m2r_dbg.ll"));

  const auto *MainFn = IRDB.getFunctionDefinition("main");
  ASSERT_NE(MainFn, nullptr);

  ValueCompressor<PAGVariable> Compressor;
  [[maybe_unused]] auto Results =
      computeAndersenOTFRaw(IRDB, {MainFn}, &Compressor);

  const auto *IdFn = IRDB.getFunctionDefinition("id");
  ASSERT_NE(IdFn, nullptr);
  auto MaybeId = Compressor.getOrNull(IdFn);
  EXPECT_TRUE(MaybeId.has_value())
      << "Function 'id' not in VC — address-taken functions must be inserted";
}

TEST(AndersenOTFAATest, ContextInsensitiveCallsMerge) {
  // context_01: id(&x) and id(&y) called from two call sites.
  // Context-insensitive: both call-site return values alias the same node
  // (pts merges both args).  A context-sensitive analysis would keep them
  // separate; this test verifies the expected context-insensitive behaviour.
  const TSL Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id"});
  const TSL Ret = TSL(RetVal{.InFunction = "id"});
  // Call instructions for id(&x) and id(&y) in main (lines 8 and 9).
  const TSL Call1 = TSL(LineColFunOp{.Line = 8,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL Call2 = TSL(LineColFunOp{.Line = 9,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const GTMap ExpectedResults = {
      {Arg, {Arg, Ret, Call1, Call2}},
      {Ret, {Arg, Ret, Call1, Call2}},
      {Call1, {Arg, Ret, Call1, Call2}},
      {Call2, {Arg, Ret, Call1, Call2}},
  };
  doAnalysisAndCheckExact("context_01_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, SeparateFunctionsDontAlias) {
  // context_02: id1 and id2 are independent identity functions called with
  // different arguments.  Their parameter and return-value nodes must not
  // alias each other (precision check for context-insensitive analysis).
  const TSL Id1Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id1"});
  const TSL Id1Ret = TSL(RetVal{.InFunction = "id1"});
  const TSL Id2Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id2"});
  const TSL Id2Ret = TSL(RetVal{.InFunction = "id2"});
  const GTMap ExpectedResults = {
      {Id1Arg, {Id1Arg, Id1Ret}},
      {Id1Ret, {Id1Arg, Id1Ret}},
      {Id2Arg, {Id2Arg, Id2Ret}},
      {Id2Ret, {Id2Arg, Id2Ret}},
  };
  doAnalysisAndCheckExact("context_02_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, TransitiveCallChain) {
  // context_03: id2(q) = id1(q).  Alias must propagate through the chain:
  // id2_arg → id1_arg → id1_ret → id2_ret.  All four must alias.
  const GTMap ExpectedResults = {
      {TSL(ArgInFun{.Idx = 0, .InFunction = "id1"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id1"}),
        TSL(RetVal{.InFunction = "id1"}),
        TSL(ArgInFun{.Idx = 0, .InFunction = "id2"}),
        TSL(RetVal{.InFunction = "id2"})}},
      {TSL(RetVal{.InFunction = "id1"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id1"}),
        TSL(RetVal{.InFunction = "id1"}),
        TSL(ArgInFun{.Idx = 0, .InFunction = "id2"}),
        TSL(RetVal{.InFunction = "id2"})}},
      {TSL(ArgInFun{.Idx = 0, .InFunction = "id2"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id1"}),
        TSL(RetVal{.InFunction = "id1"}),
        TSL(ArgInFun{.Idx = 0, .InFunction = "id2"}),
        TSL(RetVal{.InFunction = "id2"})}},
      {TSL(RetVal{.InFunction = "id2"}),
       {TSL(ArgInFun{.Idx = 0, .InFunction = "id1"}),
        TSL(RetVal{.InFunction = "id1"}),
        TSL(ArgInFun{.Idx = 0, .InFunction = "id2"}),
        TSL(RetVal{.InFunction = "id2"})}},
  };
  doAnalysisAndCheckExact("context_03_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, DeepChainTwoObjectsMerge) {
  // context_04_1: three-level identity chain (id3→id2→id1) called with both
  // &x and &y.  Context-insensitive: all params and rets of id1/id2/id3 and
  // all four call sites alias each other AND with x/y (they share x_obj or
  // y_obj as common pointee).  x and y themselves do NOT alias each other.
  const TSL Id1Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id1"});
  const TSL Id2Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id2"});
  const TSL Id3Arg = TSL(ArgInFun{.Idx = 0, .InFunction = "id3"});
  const TSL Id1Ret = TSL(RetVal{.InFunction = "id1"});
  const TSL Id2Ret = TSL(RetVal{.InFunction = "id2"});
  const TSL Id3Ret = TSL(RetVal{.InFunction = "id3"});
  const TSL XX1 = TSL(LineColFunOp{.Line = 10,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const TSL XX2 = TSL(LineColFunOp{.Line = 11,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const TSL YY1 = TSL(LineColFunOp{.Line = 12,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const TSL YY2 = TSL(LineColFunOp{.Line = 13,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  // %x / %y: the alloca pointers passed to id3; recovered as arg 0 of
  // respective call sites (operand 0 of a CallInst = first argument).
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 10,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 12,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const std::vector<TSL> Chain = {Id1Arg, Id2Arg, Id3Arg, Id1Ret, Id2Ret,
                                  Id3Ret, XX1,    XX2,    YY1,    YY2};
  // Chain members alias each other and both allocas (share x_obj or y_obj).
  std::vector<TSL> ChainWithBoth = Chain;
  ChainWithBoth.push_back(XAlloca);
  ChainWithBoth.push_back(YAlloca);
  GTMap ExpectedResults;
  for (const auto &ChainV : Chain) {
    ExpectedResults[ChainV] = ChainWithBoth;
  }
  // x alloca aliases the chain (via x_obj) but NOT y.
  std::vector<TSL> XAliases = Chain;
  XAliases.push_back(XAlloca);
  ExpectedResults[XAlloca] = XAliases;
  // y alloca aliases the chain (via y_obj) but NOT x.
  std::vector<TSL> YAliases = Chain;
  YAliases.push_back(YAlloca);
  ExpectedResults[YAlloca] = YAliases;

  // llvm::errs() << "ExpectedResults[XAlloca]: "
  //              << PrettyPrinter{ExpectedResults[XAlloca]} << '\n';
  // llvm::errs() << "ExpectedResults[YAlloca]: "
  //              << PrettyPrinter{ExpectedResults[YAlloca]} << '\n';

  doAnalysisAndCheckExact("context_04_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, RecursiveSelfAlias) {
  // context_08: selfRecursion(Ptr) calls itself with Ptr, forming a cycle in
  // the constraint graph.  SCC collapsing must merge the recursive call result
  // with the formal parameter and the two call-site results in main.
  const TSL Ptr = TSL(ArgInFun{.Idx = 0, .InFunction = "selfRecursion"});
  const TSL Ret = TSL(RetVal{.InFunction = "selfRecursion"});
  // int *x = selfRecursion(kptr)  at line 15
  const TSL X = TSL(LineColFunOp{.Line = 15,
                                 .Col = 0,
                                 .InFunction = "main",
                                 .OpCode = llvm::Instruction::Call});
  // int *y = selfRecursion(kptr)  at line 19
  const TSL Y = TSL(LineColFunOp{.Line = 19,
                                 .Col = 0,
                                 .InFunction = "main",
                                 .OpCode = llvm::Instruction::Call});
  const std::vector<TSL> All = {Ptr, Ret, X, Y};
  GTMap ExpectedResults;
  for (const auto &V : All) {
    ExpectedResults[V] = All;
  }
  doAnalysisAndCheckExact("context_08_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, MutualRecursionAlias) {
  // context_10_0: Forth and Back call each other with the same pointer; both
  // called from main with &k.  The mutual recursion forces all four
  // param/ret nodes and the two call-site results to alias.
  const TSL ForthPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Forth"});
  const TSL BackPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Back"});
  const TSL ForthRet = TSL(RetVal{.InFunction = "Forth"});
  const TSL BackRet = TSL(RetVal{.InFunction = "Back"});
  // int *x = Back(&k)  at line 26
  const TSL X = TSL(LineColFunOp{.Line = 26,
                                 .Col = 0,
                                 .InFunction = "main",
                                 .OpCode = llvm::Instruction::Call});
  // int *y = Back(&k)  at line 30
  const TSL Y = TSL(LineColFunOp{.Line = 30,
                                 .Col = 0,
                                 .InFunction = "main",
                                 .OpCode = llvm::Instruction::Call});
  const std::vector<TSL> All = {ForthPtr, BackPtr, ForthRet, BackRet, X, Y};
  GTMap ExpectedResults;
  for (const auto &V : All) {
    ExpectedResults[V] = All;
  }
  doAnalysisAndCheckExact("context_10_0_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, ReturnSecondArgContextInsensitive) {
  // context_12_1: argretq(p,q) returns q.  Two call sites swap which
  // argument is &x and which is &y.  Context-insensitive: p, q, and the
  // return value all receive both &x and &y, so they all alias each other.
  const TSL P = TSL(ArgInFun{.Idx = 0, .InFunction = "argretq"});
  const TSL Q = TSL(ArgInFun{.Idx = 1, .InFunction = "argretq"});
  const TSL Ret = TSL(RetVal{.InFunction = "argretq"});
  // int *xx1 = argretq(&y, &x)  at line 8
  const TSL XX1 = TSL(LineColFunOp{.Line = 8,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  // int *yy1 = argretq(&x, &y)  at line 9
  const TSL YY1 = TSL(LineColFunOp{.Line = 9,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const std::vector<TSL> All = {P, Q, Ret, XX1, YY1};
  GTMap ExpectedResults;
  for (const auto &V : All) {
    ExpectedResults[V] = All;
  }
  doAnalysisAndCheckExact("context_12_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FuncPtrCallbackIdentity) {
  // context_14_1: callback(Func) returns Func — identity on function pointers.
  // Two call sites pass &ret0 and &ret1 respectively.  OTF must discover
  // both callees.  The formal parameter and return value of callback must
  // alias (they point to the same set of function objects).
  const TSL Func = TSL(ArgInFun{.Idx = 0, .InFunction = "callback"});
  const TSL Ret = TSL(RetVal{.InFunction = "callback"});
  const GTMap ExpectedResults = {
      {Func, {Func, Ret}},
      {Ret, {Func, Ret}},
  };
  doAnalysisAndCheckExact("context_14_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, RecursionTwoObjectsMerge) {
  // context_09_0: selfRecursion called with &k and &l.
  // Context-insensitive: Ptr receives both; all four alias.
  // k and l alias the chain (via their objects) but not each other.
  const TSL Ptr = TSL(ArgInFun{.Idx = 0, .InFunction = "selfRecursion"});
  const TSL Ret = TSL(RetVal{.InFunction = "selfRecursion"});
  const TSL CallX = TSL(LineColFunOp{.Line = 15,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL CallY = TSL(LineColFunOp{.Line = 16,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL KAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 15,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL LAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 16,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const std::vector<TSL> Chain = {Ptr, Ret, CallX, CallY};
  GTMap ExpectedResults;
  std::vector<TSL> ChainAndBoth = Chain;
  ChainAndBoth.push_back(KAlloca);
  ChainAndBoth.push_back(LAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  std::vector<TSL> KAliases = Chain;
  KAliases.push_back(KAlloca);
  ExpectedResults[KAlloca] = KAliases;
  std::vector<TSL> LAliases = Chain;
  LAliases.push_back(LAlloca);
  ExpectedResults[LAlloca] = LAliases;
  doAnalysisAndCheckExact("context_09_0_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, MutualRecursionTwoObjects) {
  // context_10_1: Forth↔Back mutual recursion, called with &k and &l.
  // All four params/rets and four call-site results alias.
  // k and l each alias all eight but not each other.
  const TSL ForthPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Forth"});
  const TSL BackPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Back"});
  const TSL ForthRet = TSL(RetVal{.InFunction = "Forth"});
  const TSL BackRet = TSL(RetVal{.InFunction = "Back"});
  // xx1=Back(&k) line 27, xx2=Back(&k) line 29, yy1=Back(&l) line 31,
  // yy2=Back(&l) line 33
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const TSL XX1 = MkCall(27);
  const TSL XX2 = MkCall(29);
  const TSL YY1 = MkCall(31);
  const TSL YY2 = MkCall(33);
  const TSL KAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 27,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL LAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 31,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const std::vector<TSL> Chain = {ForthPtr, BackPtr, ForthRet, BackRet,
                                  XX1,      XX2,     YY1,      YY2};
  GTMap ExpectedResults;
  std::vector<TSL> ChainAndBoth = Chain;
  ChainAndBoth.push_back(KAlloca);
  ChainAndBoth.push_back(LAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  std::vector<TSL> KAliases = Chain;
  KAliases.push_back(KAlloca);
  ExpectedResults[KAlloca] = KAliases;
  std::vector<TSL> LAliases = Chain;
  LAliases.push_back(LAlloca);
  ExpectedResults[LAlloca] = LAliases;
  doAnalysisAndCheckExact("context_10_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, ThreeWayMutualRecursion) {
  // context_11_0: Forth↔Back↔Stop three-way mutual recursion.
  // All six params/rets and both call-site results alias.
  const TSL ForthPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Forth"});
  const TSL BackPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Back"});
  const TSL StopPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Stop"});
  const TSL ForthRet = TSL(RetVal{.InFunction = "Forth"});
  const TSL BackRet = TSL(RetVal{.InFunction = "Back"});
  const TSL StopRet = TSL(RetVal{.InFunction = "Stop"});
  // x=Back(&k) line 36, y=Forth(&l) line 37
  const TSL CallX = TSL(LineColFunOp{.Line = 36,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL CallY = TSL(LineColFunOp{.Line = 37,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL KAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 36,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL LAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 37,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const std::vector<TSL> Chain = {ForthPtr, BackPtr, StopPtr, ForthRet,
                                  BackRet,  StopRet, CallX,   CallY};
  GTMap ExpectedResults;
  std::vector<TSL> ChainAndBoth = Chain;
  ChainAndBoth.push_back(KAlloca);
  ChainAndBoth.push_back(LAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  std::vector<TSL> KAliases = Chain;
  KAliases.push_back(KAlloca);
  ExpectedResults[KAlloca] = KAliases;
  std::vector<TSL> LAliases = Chain;
  LAliases.push_back(LAlloca);
  ExpectedResults[LAlloca] = LAliases;
  doAnalysisAndCheckExact("context_11_0_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, ThreeArgReturnQContextInsensitive) {
  // context_13_1: argretq(p,q,r) returns q.  Two call sites pass all-x and
  // all-y.  Context-insensitive: all three params and the return merge.
  // x and y allocas alias the group but not each other.
  const TSL ArgP = TSL(ArgInFun{.Idx = 0, .InFunction = "argretq"});
  const TSL ArgQ = TSL(ArgInFun{.Idx = 1, .InFunction = "argretq"});
  const TSL ArgR = TSL(ArgInFun{.Idx = 2, .InFunction = "argretq"});
  const TSL Ret = TSL(RetVal{.InFunction = "argretq"});
  // xx1=argretq(&x,&x,&x) line 8, yy1=argretq(&y,&y,&y) line 9
  const TSL XX1 = TSL(LineColFunOp{.Line = 8,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const TSL YY1 = TSL(LineColFunOp{.Line = 9,
                                   .Col = 0,
                                   .InFunction = "main",
                                   .OpCode = llvm::Instruction::Call});
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 8,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 9,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const std::vector<TSL> Chain = {ArgP, ArgQ, ArgR, Ret, XX1, YY1};
  GTMap ExpectedResults;
  std::vector<TSL> ChainAndBoth = Chain;
  ChainAndBoth.push_back(XAlloca);
  ChainAndBoth.push_back(YAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  std::vector<TSL> XAliases = Chain;
  XAliases.push_back(XAlloca);
  ExpectedResults[XAlloca] = XAliases;
  std::vector<TSL> YAliases = Chain;
  YAliases.push_back(YAlloca);
  ExpectedResults[YAlloca] = YAliases;
  doAnalysisAndCheckExact("context_13_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FuncPtrCallbackThreeWayMerge) {
  // context_14_2: callback(Func) returns Func, called with &ret0, &ret1,
  // &ret2.  Func and Ret alias all three function values.  The individual
  // function values alias Func and Ret but NOT each other (disjoint pts sets).
  const TSL Func = TSL(ArgInFun{.Idx = 0, .InFunction = "callback"});
  const TSL Ret = TSL(RetVal{.InFunction = "callback"});
  const TSL Ret0 = TSL(FuncByName{.FuncName = "ret0"});
  const TSL Ret1 = TSL(FuncByName{.FuncName = "ret1"});
  const TSL Ret2 = TSL(FuncByName{.FuncName = "ret2"});
  const GTMap ExpectedResults = {
      {Func, {Func, Ret, Ret0, Ret1, Ret2}},
      {Ret, {Func, Ret, Ret0, Ret1, Ret2}},
      {Ret0, {Ret0, Func, Ret}},
      {Ret1, {Ret1, Func, Ret}},
      {Ret2, {Ret2, Func, Ret}},
  };
  doAnalysisAndCheckExact("context_14_2_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FourLevelChainTwoObjects) {
  // context_05_1: 4-level identity chain (id4→id3→id2→id1), called 4 times
  // with &x and &y.  All params/rets and call sites merge
  // (context-insensitive). x and y allocas alias the chain but not each other.
  const auto MkArg = [](llvm::StringRef Fn) {
    return TSL(ArgInFun{.Idx = 0, .InFunction = Fn});
  };
  const auto MkRet = [](llvm::StringRef Fn) {
    return TSL(RetVal{.InFunction = Fn});
  };
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const std::vector<TSL> Chain = {
      MkArg("id1"), MkArg("id2"), MkArg("id3"), MkArg("id4"),
      MkRet("id1"), MkRet("id2"), MkRet("id3"), MkRet("id4"),
      MkCall(11),   MkCall(12),   MkCall(13),   MkCall(14),
  };
  // arg 0 of call at line 11 is &x; arg 0 of call at line 13 is &y.
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 11,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 13,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  GTMap ExpectedResults;
  auto ChainAndBoth = Chain;
  ChainAndBoth.push_back(XAlloca);
  ChainAndBoth.push_back(YAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  auto XAliases = Chain;
  XAliases.push_back(XAlloca);
  ExpectedResults[XAlloca] = XAliases;
  auto YAliases = Chain;
  YAliases.push_back(YAlloca);
  ExpectedResults[YAlloca] = YAliases;
  doAnalysisAndCheckExact("context_05_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FourLevelChainVariantTwoObjects) {
  // context_07: foo→bar→baz→buzz 4-level identity chain, called with &x and
  // &y.  All params/rets and both call sites alias; x and y don't alias.
  const auto MkArg = [](llvm::StringRef Fn) {
    return TSL(ArgInFun{.Idx = 0, .InFunction = Fn});
  };
  const auto MkRet = [](llvm::StringRef Fn) {
    return TSL(RetVal{.InFunction = Fn});
  };
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const std::vector<TSL> Chain = {
      MkArg("buzz"), MkArg("baz"), MkArg("bar"), MkArg("foo"), MkRet("buzz"),
      MkRet("baz"),  MkRet("bar"), MkRet("foo"), MkCall(11),   MkCall(12),
  };
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 11,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 12,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  GTMap ExpectedResults;
  auto ChainAndBoth = Chain;
  ChainAndBoth.push_back(XAlloca);
  ChainAndBoth.push_back(YAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  auto XAliases = Chain;
  XAliases.push_back(XAlloca);
  ExpectedResults[XAlloca] = XAliases;
  auto YAliases = Chain;
  YAliases.push_back(YAlloca);
  ExpectedResults[YAlloca] = YAliases;
  doAnalysisAndCheckExact("context_07_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, RecursionFourCallSites) {
  // context_09_1: selfRecursion called with &k (twice) and &l (twice).
  // Context-insensitive: Ptr and Ret alias all 4 call sites.
  // k and l each alias the chain but not each other.
  const TSL Ptr = TSL(ArgInFun{.Idx = 0, .InFunction = "selfRecursion"});
  const TSL Ret = TSL(RetVal{.InFunction = "selfRecursion"});
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const std::vector<TSL> Chain = {Ptr,        Ret,        MkCall(15),
                                  MkCall(17), MkCall(18), MkCall(20)};
  const TSL KAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 15,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL LAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 18,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  GTMap ExpectedResults;
  auto ChainAndBoth = Chain;
  ChainAndBoth.push_back(KAlloca);
  ChainAndBoth.push_back(LAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  auto KAliases = Chain;
  KAliases.push_back(KAlloca);
  ExpectedResults[KAlloca] = KAliases;
  auto LAliases = Chain;
  LAliases.push_back(LAlloca);
  ExpectedResults[LAlloca] = LAliases;
  doAnalysisAndCheckExact("context_09_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, ThreeWayMutualRecursionFourCallSites) {
  // context_11_1: Forth↔Back↔Stop three-way mutual recursion, called with &k
  // (twice) and &l (twice).  All six params/rets and all four call sites alias.
  // k and l each alias the chain but not each other.
  const TSL ForthPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Forth"});
  const TSL BackPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Back"});
  const TSL StopPtr = TSL(ArgInFun{.Idx = 0, .InFunction = "Stop"});
  const TSL ForthRet = TSL(RetVal{.InFunction = "Forth"});
  const TSL BackRet = TSL(RetVal{.InFunction = "Back"});
  const TSL StopRet = TSL(RetVal{.InFunction = "Stop"});
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const std::vector<TSL> Chain = {ForthPtr,   BackPtr,   StopPtr,    ForthRet,
                                  BackRet,    StopRet,   MkCall(36), MkCall(37),
                                  MkCall(38), MkCall(39)};
  const TSL KAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 36,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL LAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 38,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  GTMap ExpectedResults;
  auto ChainAndBoth = Chain;
  ChainAndBoth.push_back(KAlloca);
  ChainAndBoth.push_back(LAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  auto KAliases = Chain;
  KAliases.push_back(KAlloca);
  ExpectedResults[KAlloca] = KAliases;
  auto LAliases = Chain;
  LAliases.push_back(LAlloca);
  ExpectedResults[LAlloca] = LAliases;
  doAnalysisAndCheckExact("context_11_1_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, TwoArgSecondRetFourCallSites) {
  // context_12_0: argretq(p,q) returns q.  Four call sites mix &x and &y:
  //   argretq(&y,&x) twice and argretq(&x,&y) twice.
  // Context-insensitive: p and q both receive {&x,&y}; all alias.
  // x and y allocas each alias the group but not each other.
  const TSL P = TSL(ArgInFun{.Idx = 0, .InFunction = "argretq"});
  const TSL Q = TSL(ArgInFun{.Idx = 1, .InFunction = "argretq"});
  const TSL Ret = TSL(RetVal{.InFunction = "argretq"});
  const auto MkCall = [](uint32_t Line) {
    return TSL(LineColFunOp{.Line = Line,
                            .Col = 0,
                            .InFunction = "main",
                            .OpCode = llvm::Instruction::Call});
  };
  const std::vector<TSL> Chain = {P,         Q,          Ret,       MkCall(8),
                                  MkCall(9), MkCall(10), MkCall(11)};
  // arg 1 of call at line 8 is &x (argretq(&y, &x)); arg 0 is &y.
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 1,
                    .Inst = LineColFunOp{.Line = 8,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 8,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  GTMap ExpectedResults;
  auto ChainAndBoth = Chain;
  ChainAndBoth.push_back(XAlloca);
  ChainAndBoth.push_back(YAlloca);
  for (const auto &Item : Chain) {
    ExpectedResults[Item] = ChainAndBoth;
  }
  auto XAliases = Chain;
  XAliases.push_back(XAlloca);
  ExpectedResults[XAlloca] = XAliases;
  auto YAliases = Chain;
  YAliases.push_back(YAlloca);
  ExpectedResults[YAlloca] = YAliases;
  doAnalysisAndCheckExact("context_12_0_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, VTableDispatch) {
  // Virtual call via A* in call_get must resolve through the vtable.
  // A::get() returns @x, so call_get's return must alias @x.
  const TSL CallGetRet = TSL(RetVal{.InFunction = "_ZL8call_getP1A"});
  const TSL X = TSL(GlobalVar{.Name = "x"});
  const GTMap ExpectedResults = {
      {CallGetRet, {CallGetRet, X}},
      {X, {X, CallGetRet}},
  };
  doAnalysisAndCheckExact("andersen_otf_vtable_cpp_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, GlobalPtrInitializer) {
  // @p = global ptr @x; loading from @p must alias @x (Bug 2 soundness).
  const TSL LoadQ = TSL(LineColFunOp{.Line = 7,
                                     .Col = 12,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Load});
  const TSL X = TSL(GlobalVar{.Name = "x"});
  const GTMap ExpectedResults = {
      {LoadQ, {LoadQ, X}},
      {X, {X, LoadQ}},
  };
  doAnalysisAndCheckExact("andersen_otf_global_init_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, MergeLoadConstraint) {
  // h->f->h cycle; h returns *p.
  // ret(h) must alias x and y after h(&px) and h(&py) (Bug 1 soundness).
  const TSL RetH = TSL(RetVal{.InFunction = "h"});
  // Operand 1 (pointer) of "int x = 0" / "int y = 0" stores — stable across
  // LLVM versions (unlike the px/py initialization stores whose debug
  // location moved from first-use to declaration site between LLVM 16 and 22).
  const TSL VarX =
      TSL(OperandOf{.OperandIndex = 1,
                    .Inst = LineColFunOp{.Line = 13,
                                         .Col = 7,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Store}});
  const TSL VarY =
      TSL(OperandOf{.OperandIndex = 1,
                    .Inst = LineColFunOp{.Line = 14,
                                         .Col = 7,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Store}});
  const GTMap ExpectedResults = {
      {RetH, {RetH, VarX, VarY}},
      {VarX, {RetH, VarX}},
      {VarY, {RetH, VarY}},
  };
  doAnalysisAndCheckExact("andersen_otf_merge_load_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, AlreadyProcessedCalleePropagation) {
  // andersen_otf_fp_already_processed: main pushes D, A, B → LIFO processes
  // B first (call2 deferred, pts={}), A second (call1 deferred, pts={}),
  // D third (relay/get_x/get_y processed, g_fp1=relay, g_fp2=get_x set).
  // checkUnresolvedFPCalls: call2 sees pts={get_x}, call1 connects already-
  // processed relay with get_y → g_fp2 gains get_y — but call2 already ran.
  // The outer loop must re-check so ret(B) aliases both &x and &y.
  const TSL RetB = TSL(RetVal{.InFunction = "B"});
  const TSL X = TSL(GlobalVar{.Name = "x"});
  const TSL Y = TSL(GlobalVar{.Name = "y"});
  const GTMap ExpectedResults = {
      {RetB, {RetB, X, Y}},
      {X, {X, RetB}},
      {Y, {Y, RetB}},
  };
  doAnalysisAndCheckExact("andersen_otf_fp_already_processed_c_dbg.ll",
                          ExpectedResults);
}

TEST(AndersenOTFAATest, VTableDispatchPrecision) {
  // B has two virtual methods: getX (slot 0) returns @x, getY (slot 1)
  // returns @y. Per-slot dispatch must keep the two return values separate.
  const TSL RetGetX = TSL(RetVal{.InFunction = "_ZL9call_getXP1B"});
  const TSL RetGetY = TSL(RetVal{.InFunction = "_ZL9call_getYP1B"});
  const TSL X = TSL(GlobalVar{.Name = "x"});
  const TSL Y = TSL(GlobalVar{.Name = "y"});
  const GTMap ExpectedResults = {
      {RetGetX, {RetGetX, X}},
      {X, {X, RetGetX}},
      {RetGetY, {RetGetY, Y}},
      {Y, {Y, RetGetY}},
  };
  doAnalysisAndCheckExact("andersen_otf_vtable2_cpp_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, SoundnessFnPtrToExternalDecl) {
  // andersen_otf_extern_callback: main passes @close_stdout to the
  // declaration-only register_callback.  close_stdout calls flush_impl.
  //
  // Soundy: both must appear as CG vertices (entry-point promotion).
  // Unsound: neither must appear (no processing of external callbacks).
  auto IRDB = LLVMProjectIRDB::loadOrExit(
      PathToLLFiles + "andersen_otf_extern_callback_c_dbg.ll");

  const auto *CloseStdout = IRDB.getFunctionDefinition("close_stdout");
  const auto *FlushImpl = IRDB.getFunctionDefinition("flush_impl");
  const auto *MainFn = IRDB.getFunctionDefinition("main");
  ASSERT_NE(MainFn, nullptr);
  ASSERT_NE(CloseStdout, nullptr);
  ASSERT_NE(FlushImpl, nullptr);

  auto HasCGVertex = [](const LLVMBasedCallGraph &Graph,
                        const llvm::Function *Fun) {
    return llvm::is_contained(Graph.getAllVertexFunctions(), Fun);
  };

  {
    auto Res =
        computeAndersenOTFRaw(IRDB, {MainFn}, nullptr, Soundness::Soundy);
    EXPECT_TRUE(HasCGVertex(Res.CG, CloseStdout))
        << "close_stdout must be a CG vertex at Soundy";
    EXPECT_TRUE(HasCGVertex(Res.CG, FlushImpl))
        << "flush_impl must be a CG vertex at Soundy";
  }

  {
    auto Res =
        computeAndersenOTFRaw(IRDB, {MainFn}, nullptr, Soundness::Unsound);
    EXPECT_FALSE(HasCGVertex(Res.CG, CloseStdout))
        << "close_stdout must not be a CG vertex at Unsound";
    EXPECT_FALSE(HasCGVertex(Res.CG, FlushImpl))
        << "flush_impl must not be a CG vertex at Unsound";
  }
}

TEST(AndersenOTFAATest, LibCSummaryStrcpyReturnAliasesDst) {
  // strcpy(buf, "hello") summary: param 0 (dst) -> ReturnValue.
  // The call result must alias buf (arg 0); they share the same buffer object.
  // This exercises the ReturnValue branch of applyLibrarySummary().
  const TSL Call = TSL(LineColFunOp{.Line = 9,
                                    .Col = 0,
                                    .InFunction = "main",
                                    .OpCode = llvm::Instruction::Call});
  const TSL Buf =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 9,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const GTMap ExpectedResults = {
      {Call, {Call, Buf}},
      {Buf, {Buf, Call}},
  };
  doAnalysisAndCheckExact("andersen_otf_libc_c_m2r_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, FnPtrStoredInStructField) {
  // Function pointer stored into a struct field by an initializer, then
  // retrieved and called indirectly.  The indirect call in do_call() must
  // have target() as a callee.
  auto IRDB = LLVMProjectIRDB::loadOrExit(
      PathToLLFiles + "andersen_otf_fp_struct_field_c_dbg.ll");

  const auto *DoCall = IRDB.getFunctionDefinition("do_call");
  const auto *Target = IRDB.getFunctionDefinition("target");
  const auto *MainFn = IRDB.getFunctionDefinition("main");
  ASSERT_NE(MainFn, nullptr);
  ASSERT_NE(DoCall, nullptr);
  ASSERT_NE(Target, nullptr);

  auto Res = computeAndersenOTFRaw(IRDB, {MainFn});

  // Find the indirect call instruction in do_call.
  const llvm::CallBase *IndirectCS = nullptr;
  for (const auto &I : llvm::instructions(DoCall)) {
    const auto *CS = llvm::dyn_cast<llvm::CallBase>(&I);
    if (!CS || CS->isDebugOrPseudoInst()) {
      continue;
    }
    if (!llvm::isa<llvm::Function>(
            CS->getCalledOperand()->stripPointerCastsAndAliases())) {
      IndirectCS = CS;
      break;
    }
  }
  ASSERT_NE(IndirectCS, nullptr) << "No indirect call found in do_call";

  const auto &Callees = Res.CG.getCalleesOfCallAt(IndirectCS);
  EXPECT_TRUE(llvm::is_contained(Callees, Target))
      << "target() must be a callee of the indirect call in do_call()";
}

TEST(AndersenOTFAATest, StructVtableDispatch) {
  // Hand-rolled C vtable: const struct Ops { read, write }.
  // ops->write(...) must resolve to myWrite only, not myRead.
  // Without the struct-vtable path, field-insensitive analysis adds both.
  auto IRDB = LLVMProjectIRDB::loadOrExit(
      PathToLLFiles + "andersen_otf_struct_vtable_c_m2r_dbg.ll");

  const auto *DispatchFn = IRDB.getFunctionDefinition("dispatch");
  const auto *MyRead = IRDB.getFunctionDefinition("myRead");
  const auto *MyWrite = IRDB.getFunctionDefinition("myWrite");
  const auto *MainFn = IRDB.getFunctionDefinition("main");
  ASSERT_NE(MainFn, nullptr);
  ASSERT_NE(DispatchFn, nullptr);
  ASSERT_NE(MyRead, nullptr);
  ASSERT_NE(MyWrite, nullptr);

  auto Res = computeAndersenOTFRaw(IRDB, {MainFn});

  const llvm::CallBase *IndirectCS = nullptr;
  for (const auto &I : llvm::instructions(DispatchFn)) {
    const auto *CS = llvm::dyn_cast<llvm::CallBase>(&I);
    if (!CS || CS->isDebugOrPseudoInst()) {
      continue;
    }
    if (!llvm::isa<llvm::Function>(
            CS->getCalledOperand()->stripPointerCastsAndAliases())) {
      IndirectCS = CS;
      break;
    }
  }
  ASSERT_NE(IndirectCS, nullptr) << "No indirect call found in dispatch()";

  const auto &Callees = Res.CG.getCalleesOfCallAt(IndirectCS);
  EXPECT_TRUE(llvm::is_contained(Callees, MyWrite))
      << "myWrite must be a callee of ops->write(...)";
  EXPECT_FALSE(llvm::is_contained(Callees, MyRead))
      << "myRead must not be a callee of ops->write(...) (field 1, not 0)";
}

TEST(AndersenOTFAATest, AllocWrapperCallSitesDontAlias) {
  // factory_01: factory_fun mallocs and returns Mem. Two call sites in
  // main must get distinct abstract objects, not the wrapper's shared
  // internal allocation site.
  const TSL Call1 = TSL(LineColFunOp{.Line = 18,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const TSL Call2 = TSL(LineColFunOp{.Line = 19,
                                     .Col = 0,
                                     .InFunction = "main",
                                     .OpCode = llvm::Instruction::Call});
  const GTMap ExpectedResults = {
      {Call1, {Call1}},
      {Call2, {Call2}},
  };
  doAnalysisAndCheckExact("factory_01_c_dbg.ll", ExpectedResults);
}

TEST(AndersenOTFAATest, EscapingAllocWrapperStaysMerged) {
  // factory_02: makeSlot() mallocs but also passes the pointer to setPtr(),
  // which writes through it before returning. Must not be classified as a
  // plain wrapper, else the write would be lost (Fld1/Fld2 would come back
  // empty instead of aliasing &X/&Y).
  const TSL XAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 17,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  const TSL YAlloca =
      TSL(OperandOf{.OperandIndex = 0,
                    .Inst = LineColFunOp{.Line = 18,
                                         .Col = 0,
                                         .InFunction = "main",
                                         .OpCode = llvm::Instruction::Call}});
  // Fld1 = *P1, Fld2 = *P2 (field loads, col 13; col 14 is the P1/P2 load).
  const TSL Fld1 = TSL(LineColFunOp{.Line = 19,
                                    .Col = 13,
                                    .InFunction = "main",
                                    .OpCode = llvm::Instruction::Load});
  const TSL Fld2 = TSL(LineColFunOp{.Line = 20,
                                    .Col = 13,
                                    .InFunction = "main",
                                    .OpCode = llvm::Instruction::Load});
  const std::vector<TSL> All = {XAlloca, YAlloca, Fld1, Fld2};
  const GTMap ExpectedResults = {
      {XAlloca, {XAlloca, Fld1, Fld2}},
      {YAlloca, {YAlloca, Fld1, Fld2}},
      {Fld1, All},
      {Fld2, All},
  };
  doAnalysisAndCheckExact("factory_02_c_dbg.ll", ExpectedResults);
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
