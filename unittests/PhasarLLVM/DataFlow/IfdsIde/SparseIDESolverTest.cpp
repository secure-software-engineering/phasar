#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/ControlFlow/SparseCFGProvider.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypeTraits.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <tuple>

using namespace psr;
namespace {
/* ============== TEST FIXTURE ============== */
class LinearConstant : public ::testing::TestWithParam<std::string_view> {
protected:
  const std::vector<std::string> EntryPoints = {"main"};
};
class DoubleFreeTA : public ::testing::TestWithParam<std::string_view> {
protected:
  const std::vector<std::string> EntryPoints = {"main"};
};

TEST_P(LinearConstant, SparseResultsEquivalent) {
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");
  LLVMProjectIRDB IRDB(PathToLlFiles + GetParam());
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);

  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH, &PT);
  auto HasGlobalCtor = IRDB.getFunctionDefinition(
                           LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;
  std::vector Entry = {
      HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str() : "main"};
  SparseLLVMBasedICFG SICF(&IRDB, CallGraphAnalysisType::OTF, Entry, &TH, &PT,
                           psr::Soundness::Soundy, false);

  static_assert(has_getSparseCFG_v<SparseLLVMBasedICFG, const llvm::Value *>);

  IDELinearConstantAnalysis LCAProblem(&IRDB, &ICF, Entry);
  IDELinearConstantAnalysis SLCAProblem(&IRDB, &SICF, Entry);

  auto DenseResults = IDESolver(LCAProblem, &ICF).solve();
  auto SparseResults = IDESolver(SLCAProblem, &SICF).solve();

  DenseResults.dumpResults(ICF, llvm::outs() << "DenseResults:");
  SparseResults.dumpResults(SICF, llvm::outs() << "SparseResults:");

  for (auto &&Cell : SparseResults.getAllResultEntries()) {
    auto DenseRes =
        DenseResults.resultAt(Cell.getRowKey(), Cell.getColumnKey());
    EXPECT_EQ(DenseRes, Cell.getValue())
        << "At " << llvmIRToString(Cell.getRowKey())
        << " :: " << llvmIRToShortString(Cell.getColumnKey());
  }

  // TODO: Check for existing results
}

static LLVMTaintConfig getDoubleFreeConfig() {
  auto SourceCB = [](const llvm::Instruction *Inst) {
    std::set<const llvm::Value *> Ret;
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
        Call && Call->getCalledFunction() &&
        Call->getCalledFunction()->getName() == "free") {
      Ret.insert(Call->getArgOperand(0));
    }
    return Ret;
  };

  return LLVMTaintConfig(SourceCB, SourceCB);
}

TEST_P(DoubleFreeTA, SparseLeaksEquivalent) {
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("taint_analysis/");
  LLVMProjectIRDB IRDB(PathToLlFiles + GetParam());
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);

  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH, &PT);
  auto HasGlobalCtor = IRDB.getFunctionDefinition(
                           LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;
  std::vector Entry = {
      HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str() : "main"};
  SparseLLVMBasedICFG SICF(&IRDB, CallGraphAnalysisType::OTF, Entry, &TH, &PT,
                           psr::Soundness::Soundy, false);

  static_assert(has_getSparseCFG_v<SparseLLVMBasedICFG, const llvm::Value *>);

  auto Config = getDoubleFreeConfig();
  IFDSTaintAnalysis TaintProblem(&IRDB, &PT, &Config, Entry);
  IFDSTaintAnalysis STaintProblem(&IRDB, &PT, &Config, Entry);

  auto DenseResults = IDESolver(TaintProblem, &ICF).solve();
  auto SparseResults = IDESolver(STaintProblem, &SICF).solve();

  for (const auto &[LeakInst, Leaks] : TaintProblem.Leaks) {
    auto LeakIt = STaintProblem.Leaks.find(LeakInst);
    EXPECT_NE(LeakIt, STaintProblem.Leaks.end())
        << "SparseIDE did not find expected leak(s) at "
        << llvmIRToString(LeakInst);

    if (LeakIt == STaintProblem.Leaks.end()) {
      continue;
    }

    const auto &SLeaks = LeakIt->second;
    EXPECT_EQ(Leaks, SLeaks)
        << "Leak sets at " << llvmIRToString(LeakInst) << " do not match";
  }
}

static constexpr std::string_view LCATestFiles[] = {
    "basic_01_cpp_dbg.ll",
    "basic_02_cpp_dbg.ll",
    "basic_03_cpp_dbg.ll",
    "basic_04_cpp_dbg.ll",
    "basic_05_cpp_dbg.ll",
    "basic_06_cpp_dbg.ll",
    "basic_07_cpp_dbg.ll",
    "basic_08_cpp_dbg.ll",
    "basic_09_cpp_dbg.ll",
    "basic_10_cpp_dbg.ll",
    "basic_11_cpp_dbg.ll",
    "basic_12_cpp_dbg.ll",

    "branch_01_cpp_dbg.ll",
    "branch_02_cpp_dbg.ll",
    "branch_03_cpp_dbg.ll",
    "branch_04_cpp_dbg.ll",
    "branch_05_cpp_dbg.ll",
    "branch_06_cpp_dbg.ll",
    "branch_07_cpp_dbg.ll",

    "while_01_cpp_dbg.ll",
    "while_02_cpp_dbg.ll",
    "while_03_cpp_dbg.ll",
    "while_04_cpp_dbg.ll",
    "while_05_cpp_dbg.ll",
    "for_01_cpp_dbg.ll",

    "call_01_cpp_dbg.ll",
    "call_02_cpp_dbg.ll",
    "call_03_cpp_dbg.ll",
    "call_04_cpp_dbg.ll",
    "call_05_cpp_dbg.ll",
    "call_06_cpp_dbg.ll",
    "call_07_cpp_dbg.ll",
    "call_08_cpp_dbg.ll",
    "call_09_cpp_dbg.ll",
    "call_10_cpp_dbg.ll",
    "call_11_cpp_dbg.ll",

    "recursion_01_cpp_dbg.ll",
    "recursion_02_cpp_dbg.ll",
    "recursion_03_cpp_dbg.ll",

    "global_01_cpp_dbg.ll",
    "global_02_cpp_dbg.ll",
    "global_03_cpp_dbg.ll",
    "global_04_cpp_dbg.ll",
    "global_05_cpp_dbg.ll",
    "global_06_cpp_dbg.ll",
    "global_07_cpp_dbg.ll",
    "global_08_cpp_dbg.ll",
    "global_09_cpp_dbg.ll",
    "global_10_cpp_dbg.ll",
    "global_11_cpp_dbg.ll",
    "global_12_cpp_dbg.ll",
    "global_13_cpp_dbg.ll",
    "global_14_cpp_dbg.ll",
    "global_15_cpp_dbg.ll",
    "global_16_cpp_dbg.ll",

    "overflow_add_cpp_dbg.ll",
    "overflow_sub_cpp_dbg.ll",
    "overflow_mul_cpp_dbg.ll",
    "overflow_div_min_by_neg_one_cpp_dbg.ll",

    "ub_division_by_zero_cpp_dbg.ll",
    "ub_modulo_by_zero_cpp_dbg.ll",
};

static constexpr std::string_view TaintTestFiles[] = {
    "double_free_01_c.ll",
    "double_free_02_c.ll",
};

INSTANTIATE_TEST_SUITE_P(SparseIDETest, LinearConstant,
                         ::testing::ValuesIn(LCATestFiles));
INSTANTIATE_TEST_SUITE_P(SparseIDETest, DoubleFreeTA,
                         ::testing::ValuesIn(TaintTestFiles));
} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
