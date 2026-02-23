
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"

#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/StaticIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Printer.h"

#include "llvm/IR/IntrinsicInst.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <type_traits>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IterativeIDESolverTest
    : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");

  template <typename SolverConfigTy = IDESolverConfig>
  void doAnalysis(const llvm::Twine &LlvmFilePath, bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLlFiles + LlvmFilePath);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                       Soundness::Soundy, /*IncludeGlobals*/ true);

    IDELinearConstantAnalysis Problem(&IRDB, &ICFG, {"main"});
    IterativeIDESolver<IDELinearConstantAnalysis, SolverConfigTy> Solver(
        &Problem, &ICFG);

    auto Start = std::chrono::steady_clock::now();
    Solver.solve();
    auto End = std::chrono::steady_clock::now();
    auto NewTime = End - Start;
    llvm::errs() << "IterativeIDESolver Elapsed:\t" << NewTime.count()
                 << "ns\n";

    IDESolver OldSolver(&Problem, &ICFG);
    Start = std::chrono::steady_clock::now();
    OldSolver.solve();
    End = std::chrono::steady_clock::now();

    auto OldTime = End - Start;
    llvm::errs() << "IDESolver Elapsed:\t\t" << OldTime.count() << "ns\n";

    if (PrintDump) {
      Solver.dumpResults();
      OldSolver.dumpResults();
    }

    checkEquality(OldSolver.getSolverResults(), Solver.getSolverResults(),
                  SolverConfigTy{});

    [[maybe_unused]] GenericSolverResults<const llvm::Instruction *,
                                          const llvm::Value *,
                                          LatticeDomain<int64_t>> SR =
        OldSolver.getSolverResults();
    [[maybe_unused]] GenericSolverResults<
        const llvm::Instruction *, const llvm::Value *,
        std::conditional_t<SolverConfigTy::ComputeValues,
                           LatticeDomain<int64_t>, BinaryDomain>> SR2 =
        Solver.getSolverResults();
  }

  // Check that the IDESolverAPIMixin works correctly
  template <typename SolverConfigTy = IDESolverConfig>
  void doAnalysisWithAPIMixin(const llvm::Twine &LlvmFilePath,
                              bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLlFiles + LlvmFilePath);
    DIBasedTypeHierarchy TH(IRDB);
    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                       Soundness::Soundy, /*IncludeGlobals*/ true);

    IDELinearConstantAnalysis Problem(&IRDB, &ICFG, {"main"});
    IterativeIDESolver<IDELinearConstantAnalysis, SolverConfigTy> Solver(
        &Problem, &ICFG);

    auto Start = std::chrono::steady_clock::now();
    std::atomic_bool Cancel = false;
    auto _ = Solver.solveWithAsyncCancellation(Cancel);
    auto End = std::chrono::steady_clock::now();
    auto NewTime = End - Start;
    llvm::errs() << "IterativeIDESolver Elapsed:\t" << NewTime.count()
                 << "ns\n";

    IDESolver OldSolver(&Problem, &ICFG);
    Start = std::chrono::steady_clock::now();
    auto OldResults = OldSolver.solve();
    End = std::chrono::steady_clock::now();

    auto OldTime = End - Start;
    llvm::errs() << "IDESolver Elapsed:\t\t" << OldTime.count() << "ns\n";

    if (PrintDump) {
      Solver.dumpResults();
      OldSolver.dumpResults();
    }

    checkEquality(OldResults, Solver.consumeSolverResults(), SolverConfigTy{});
  }

  struct NonGCIFDSSolverConfig : IFDSSolverConfig {
    static inline constexpr auto EnableJumpFunctionGC =
        JumpFunctionGCMode::Disabled;
  };

  template <typename SR1, typename SR2>
  void checkEquality(const SR1 &LHS, const SR2 &RHS, IDESolverConfig /*Tag*/) {
    llvm::errs() << "IDE Equality Check\n";

    for (const auto &[Row, ColVal] : LHS.rowMapView()) {
      EXPECT_TRUE(RHS.containsNode(Row))
          << "The RHS does not contain results at inst " << llvmIRToString(Row);

      auto RHSColVal = RHS.row(Row);
      EXPECT_EQ(ColVal.size(), RHSColVal.size())
          << "The number of dataflow facts at inst " << llvmIRToString(Row)
          << " do not match";

      for (const auto &[Col, Val] : ColVal) {
        auto It = RHSColVal.find(Col);

        EXPECT_TRUE(It != RHSColVal.end())
            << "The RHS does not contain fact " << llvmIRToString(Col)
            << " at inst " << llvmIRToString(Row);
        if (It != RHSColVal.end()) {
          EXPECT_TRUE(Val == It->second)
              << "The edge values at inst " << llvmIRToString(Row)
              << " and fact " << llvmIRToString(Col) << " do not match: " << Val
              << " vs " << It->second;
        }
      }
    }
    for (const auto &[Row, ColVal] : RHS.getAllResultEntries()) {
      if (llvm::isa<llvm::DbgInfoIntrinsic>(Row)) {
        continue;
      }
      EXPECT_TRUE(LHS.containsNode(Row))
          << "The old results do not contain Node " << NToString(Row);
    }
  }

  template <typename SR1, typename SR2>
  void checkEquality(const SR1 &LHS, const SR2 &RHS, IFDSSolverConfig /*Tag*/) {
    llvm::errs() << "IFDS Equality Check\n";
    EXPECT_EQ(LHS.size(), RHS.size())
        << "The instructions, where results are computed differ";

    for (const auto &[Row, ColVal] : LHS.rowMapView()) {
      EXPECT_TRUE(RHS.containsNode(Row))
          << "The RHS does not contain results at inst " << llvmIRToString(Row);

      auto RHSColVal = RHS.row(Row);
      EXPECT_EQ(ColVal.size(), RHSColVal.size())
          << "The number of dataflow facts at inst " << llvmIRToString(Row)
          << " do not match";

      for (const auto &[Col, Val] : ColVal) {
        EXPECT_TRUE(RHSColVal.count(Col))
            << "The RHS does not contain fact " << llvmIRToString(Col)
            << " at inst " << llvmIRToString(Row);
      }
    }
  }

  void TearDown() override {}

}; // Test Fixture

// Using IDESolverConfig
TEST_P(IterativeIDESolverTest, IDESolverTestLCA) { doAnalysis(GetParam()); }

TEST_P(IterativeIDESolverTest, IDESolverTestLCAAPIMixin) {
  doAnalysisWithAPIMixin(GetParam());
}

// Using IFDSSolverConfig
TEST_P(IterativeIDESolverTest, IFDSSolverTestLCA) {
  doAnalysis<IFDSSolverConfig>(GetParam());
}

TEST_P(IterativeIDESolverTest, IFDSSolverTestLCAAPIMixin) {
  doAnalysisWithAPIMixin<IFDSSolverConfig>(GetParam());
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
    "external_fun_cpp.ll",
};

INSTANTIATE_TEST_SUITE_P(IterativeIDESolverTest, IterativeIDESolverTest,
                         ::testing::ValuesIn(LCATestFiles));

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
