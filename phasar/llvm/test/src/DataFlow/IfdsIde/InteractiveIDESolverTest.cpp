#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/Utils/TypeTraits.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <tuple>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class LinearConstant : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");
  const std::vector<std::string> EntryPoints = {"main"};

}; // Test Fixture

TEST_P(LinearConstant, ResultsEquivalentSolveUntil) {
  HelperAnalyses HA(PathToLlFiles + GetParam(), EntryPoints);

  // Compute the ICFG to possibly create the runtime model
  auto &ICFG = HA.getICFG();

  auto HasGlobalCtor = HA.getProjectIRDB().getFunctionDefinition(
                           LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;

  auto LCAProblem = createAnalysisProblem<IDELinearConstantAnalysis>(
      HA,
      std::vector{HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str()
                                : "main"});

  auto AtomicResults = IDESolver(LCAProblem, &ICFG).solve();
  {
    auto InteractiveResults =
        IDESolver(LCAProblem, &ICFG).solveUntil(FalseFn{});

    ASSERT_TRUE(InteractiveResults.has_value());
    for (auto &&Cell : AtomicResults.getAllResultEntries()) {
      auto InteractiveRes =
          InteractiveResults->resultAt(Cell.getRowKey(), Cell.getColumnKey());
      EXPECT_EQ(InteractiveRes, Cell.getValue());
    }
  }
  auto InterruptedResults = [&] {
    IDESolver Solver(LCAProblem, &ICFG);
    auto Result = Solver.solveUntil(TrueFn{});
    EXPECT_EQ(std::nullopt, Result);
    if (!Result) {
      return std::move(Solver).continueSolving();
    }
    return Solver.consumeSolverResults();
  }();

  for (auto &&Cell : AtomicResults.getAllResultEntries()) {
    auto InteractiveRes =
        InterruptedResults.resultAt(Cell.getRowKey(), Cell.getColumnKey());
    EXPECT_EQ(InteractiveRes, Cell.getValue());
  }
}

TEST_P(LinearConstant, ResultsEquivalentSolveUntilAsync) {
  HelperAnalyses HA(PathToLlFiles + GetParam(), EntryPoints);

  // Compute the ICFG to possibly create the runtime model
  auto &ICFG = HA.getICFG();

  auto HasGlobalCtor = HA.getProjectIRDB().getFunctionDefinition(
                           LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;

  auto LCAProblem = createAnalysisProblem<IDELinearConstantAnalysis>(
      HA,
      std::vector{HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str()
                                : "main"});

  auto AtomicResults = IDESolver(LCAProblem, &ICFG).solve();

  {
    std::atomic_bool IsCancelled = false;
    auto InteractiveResults =
        IDESolver(LCAProblem, &ICFG).solveWithAsyncCancellation(IsCancelled);

    ASSERT_TRUE(InteractiveResults.has_value());
    for (auto &&Cell : AtomicResults.getAllResultEntries()) {
      auto InteractiveRes =
          InteractiveResults->resultAt(Cell.getRowKey(), Cell.getColumnKey());
      EXPECT_EQ(InteractiveRes, Cell.getValue());
    }
  }
  auto InterruptedResults = [&] {
    IDESolver Solver(LCAProblem, &ICFG);
    std::atomic_bool IsCancelled = true;
    auto Result = Solver.solveWithAsyncCancellation(IsCancelled);
    EXPECT_EQ(std::nullopt, Result);
    if (!Result) {
      IsCancelled = false;
      return std::move(Solver).solveWithAsyncCancellation(IsCancelled).value();
    }
    return Solver.consumeSolverResults();
  }();

  for (auto &&Cell : AtomicResults.getAllResultEntries()) {
    auto InteractiveRes =
        InterruptedResults.resultAt(Cell.getRowKey(), Cell.getColumnKey());
    EXPECT_EQ(InteractiveRes, Cell.getValue());
  }
}

static constexpr std::string_view LCATestFiles[] = {
    "basic_01.dbg.ll",
    "basic_02.dbg.ll",
    "basic_03.dbg.ll",
    "basic_04.dbg.ll",
    "basic_05.dbg.ll",
    "basic_06.dbg.ll",
    "basic_07.dbg.ll",
    "basic_08.dbg.ll",
    "basic_09.dbg.ll",
    "basic_10.dbg.ll",
    "basic_11.dbg.ll",
    "basic_12.dbg.ll",

    "branch_01.dbg.ll",
    "branch_02.dbg.ll",
    "branch_03.dbg.ll",
    "branch_04.dbg.ll",
    "branch_05.dbg.ll",
    "branch_06.dbg.ll",
    "branch_07.dbg.ll",

    "while_01.dbg.ll",
    "while_02.dbg.ll",
    "while_03.dbg.ll",
    "while_04.dbg.ll",
    "while_05.dbg.ll",
    "for_01.dbg.ll",

    "call_01.dbg.ll",
    "call_02.dbg.ll",
    "call_03.dbg.ll",
    "call_04.dbg.ll",
    "call_05.dbg.ll",
    "call_06.dbg.ll",
    "call_07.dbg.ll",
    "call_08.dbg.ll",
    "call_09.dbg.ll",
    "call_10.dbg.ll",
    "call_11.dbg.ll",

    "recursion_01.dbg.ll",
    "recursion_02.dbg.ll",
    "recursion_03.dbg.ll",

    "global_01.dbg.ll",
    "global_02.dbg.ll",
    "global_03.dbg.ll",
    "global_04.dbg.ll",
    "global_05.dbg.ll",
    "global_06.dbg.ll",
    "global_07.dbg.ll",
    "global_08.dbg.ll",
    "global_09.dbg.ll",
    "global_10.dbg.ll",
    "global_11.dbg.ll",
    "global_12.dbg.ll",
    "global_13.dbg.ll",
    "global_14.dbg.ll",
    "global_15.dbg.ll",
    "global_16.dbg.ll",

    "overflow_add.dbg.ll",
    "overflow_sub.dbg.ll",
    "overflow_mul.dbg.ll",
    "overflow_div_min_by_neg_one.dbg.ll",

    "ub_division_by_zero.dbg.ll",
    "ub_modulo_by_zero.dbg.ll",
};

INSTANTIATE_TEST_SUITE_P(InteractiveIDESolverTest, LinearConstant,
                         ::testing::ValuesIn(LCATestFiles));
