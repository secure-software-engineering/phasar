#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
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
  for (auto VId : iota<ValueId>(Results.NumVars)) {
    if (!Results.AliasSets.inbounds(VId)) {
      continue;
    }

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

      if (Results.AliasSets[VId].empty()) {
        llvm::errs() << " aliases: EMPTY\n";
        continue;
      }

      llvm::errs() << " aliases: {\n";
      Results.AliasSets[VId].foreach ([&](ValueId AId) {
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

  auto Compressor = std::make_unique<ValueCompressor<PAGVariable>>();
  AndersenOTFResult Results =
      computeAndersenOTFRaw(IRDB, Entries, Compressor.get());

  // Build domain from all values explicitly named in the GT.
  llvm::SmallDenseSet<ValueId, 16> Domain;
  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    Domain.insert(asId(*Compressor, IRDB, PtrVar));
    for (const auto &AliasVar : ExpectedAliasVars) {
      Domain.insert(asId(*Compressor, IRDB, AliasVar));
    }
  }

  for (const auto &[PtrVar, ExpectedAliasVars] : ExpectedResults) {
    const auto PtrId = asId(*Compressor, IRDB, PtrVar);
    const RawAliasSet<ValueId> &Computed = Results.getRawAliasSet(PtrId);

    RawAliasSet<ValueId> Expected;
    for (const auto &AliasVar : ExpectedAliasVars) {
      Expected.insert(asId(*Compressor, IRDB, AliasVar));
    }

    // Soundness.
    Expected.foreach ([&](ValueId AliasId) {
      if (!Computed.contains(AliasId)) {
        ADD_FAILURE_AT(Loc.file_name(), Loc.line())
            << "Missing expected alias of " << PtrVar << ": "
            << stringifyVal(*Compressor, AliasId);
      }
    });

    // Precision (domain-restricted).
    Computed.foreach ([&](ValueId VId) {
      if (!Domain.contains(VId) || Expected.contains(VId)) {
        return;
      }
      ADD_FAILURE_AT(Loc.file_name(), Loc.line())
          << "Unexpected alias of " << PtrVar << ": "
          << stringifyVal(*Compressor, VId);
    });
  }

  if (DumpResults || ::testing::Test::HasFailure()) {
    dumpAnalysisState(*Compressor, Results);
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

  auto Compressor = std::make_unique<ValueCompressor<PAGVariable>>();
  [[maybe_unused]] auto Results =
      computeAndersenOTFRaw(IRDB, {MainFn}, Compressor.get());

  const auto *IdFn = IRDB.getFunctionDefinition("id");
  ASSERT_NE(IdFn, nullptr);
  auto MaybeId = Compressor->getOrNull(IdFn);
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
  doAnalysisAndCheckExact("context_01_c_dbg.ll", ExpectedResults, true);
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

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
