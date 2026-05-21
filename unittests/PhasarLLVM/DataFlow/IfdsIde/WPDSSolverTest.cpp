
#include "phasar/DataFlow/WPDS/Solver/WPDSSolver.h"

#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/DataFlow/WPDS/IfdsIdeRuleProvider.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/SemiRing.h"
#include "phasar/Utils/Timer.h"

#include "llvm/IR/IntrinsicInst.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <chrono>
#include <type_traits>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class WPDSSolverTest : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");

  template <bool DoIDEAnalysis>
  auto doWPDSAnalysis(auto &Problem, auto &ICFG,
                      std::bool_constant<DoIDEAnalysis> ComputeWeights) {
    wpds::IfdsIdeRuleProvider LCARules(&Problem, &ICFG, ComputeWeights);
    Timer Tm([](auto Elapsed) {
      llvm::errs() << "WPDSSolver Elapsed:\t\t" << Elapsed.count() << "ns\n";
    });
    if constexpr (DoIDEAnalysis) {
      WPDSSolver NewSolver(&LCARules, &Problem);
      NewSolver.solve();
      return NewSolver.consumeSolverResults();
    } else {
      WPDSSolver NewSolver(&LCARules, &BinarySemiRing::Instance);
      NewSolver.solve();
      return NewSolver.consumeSolverResults();
    }
  }

  template <bool DoIDEAnalysis = true>
  void doAnalysis(const llvm::Twine &LlvmFilePath, bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLlFiles + LlvmFilePath);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                       Soundness::Soundy, /*IncludeGlobals*/ true);

    IDELinearConstantAnalysis Problem(&IRDB, &ICFG, {"main"});
    IterativeIDESolver BaselineSolver(&Problem, &ICFG);

    auto Start = std::chrono::steady_clock::now();
    BaselineSolver.solve();
    auto End = std::chrono::steady_clock::now();
    auto BaselineTime = End - Start;
    llvm::errs() << "IterativeIDESolver Elapsed:\t" << BaselineTime.count()
                 << "ns\n";

    auto NewResults =
        doWPDSAnalysis(Problem, ICFG, std::bool_constant<DoIDEAnalysis>{});

    if (PrintDump) {
      BaselineSolver.dumpResults();
      NewResults.dumpResults();
    }

    checkEquality(BaselineSolver.getSolverResults(), NewResults, Problem,
                  std::bool_constant<DoIDEAnalysis>{});
  }

  template <typename SR1, typename SR2, typename ProblemT, bool CompareValues>
  void checkEquality(const SR1 &LHS, const SR2 &RHS, ProblemT &Problem,
                     std::bool_constant<CompareValues>) {
    for (const auto &[Row, ColVal] : LHS.getAllResultEntries()) {
      for (const auto &[Col, Val] : ColVal) {
        bool Holds = RHS.isInResultSet(Col, Row);
        EXPECT_TRUE(Holds) << "The RHS does not contain fact "
                           << llvmIRToString(Col) << " at inst "
                           << llvmIRToString(Row);
        if constexpr (CompareValues) {
          auto RHSWeight = RHS.computeWeightAt(Col, Row, Problem);
          auto RHSVal = RHSWeight.computeTarget(Bottom{});

          EXPECT_TRUE(Val == RHSVal)
              << "The edge values at inst " << llvmIRToString(Row)
              << " and fact " << llvmIRToString(Col) << " do not match: " << Val
              << " vs " << RHSVal;
        }
      }
    }
    for (const auto &[ColVal, Row] : RHS.getAllIFDSResultEntries()) {
      if (llvm::isa<llvm::DbgInfoIntrinsic>(Row)) {
        continue;
      }
      EXPECT_TRUE(LHS.containsNode(Row))
          << "The old results do not contain Node " << NToString(Row);
    }
  }

  void TearDown() override {}

}; // Test Fixture

// Using IDESolverConfig
TEST_P(WPDSSolverTest, IDESolverTestLCA) { doAnalysis(GetParam()); }
TEST_P(WPDSSolverTest, IFDSSolverTestLCA) { doAnalysis<false>(GetParam()); }

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
    "external_fun_cpp.ll",
};

INSTANTIATE_TEST_SUITE_P(WPDSSolverTest, WPDSSolverTest,
                         ::testing::ValuesIn(LCATestFiles));

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
