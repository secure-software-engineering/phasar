
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"

#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/StaticIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <chrono>
#include <type_traits>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IterativeIDESolverTest : public ::testing::Test {
protected:
  // const std::string PathToLlFiles =
  //     unittest::PathToLLTestFiles + "control_flow/";

  // void SetUp() override {
  // boost::log::core::get()->set_logging_enabled(false); }

  template <typename SolverConfigTy = IDESolverConfig>
  void doAnalysis(const llvm::Twine &LlvmFilePath, bool PrintDump = false) {
    LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles + LlvmFilePath);
    LLVMTypeHierarchy TH(IRDB);
    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                       Soundness::Soundy, /*IncludeGlobals*/ true);

    IDELinearConstantAnalysis Problem(&IRDB, &ICFG, {"main"});
    /// Unfortunately, the legacy solver does not compute any SolverResults if
    /// value computation is turned off
    // Problem.getIFDSIDESolverConfig().setComputeValues(false);
    // Problem.getIFDSIDESolverConfig().setAutoAddZero(false);

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

    // EXPECT_TRUE(
    //     Solver.getSolverResults().ifdsEqualWith(OldSolver.getSolverResults()));
    checkEquality(OldSolver.getSolverResults(), Solver.getSolverResults(),
                  SolverConfigTy{});

    [[maybe_unused]] GenericSolverResults<
        const llvm::Instruction *, const llvm::Value *, LatticeDomain<int64_t>>
        SR = OldSolver.getSolverResults();
    [[maybe_unused]] GenericSolverResults<
        const llvm::Instruction *, const llvm::Value *,
        std::conditional_t<SolverConfigTy::ComputeValues,
                           LatticeDomain<int64_t>, BinaryDomain>>
        SR2 = Solver.getSolverResults();
  }

  struct NonGCIFDSSolverConfig : IFDSSolverConfig {
    static inline constexpr auto EnableJumpFunctionGC =
        JumpFunctionGCMode::Disabled;
  };

  template <typename SR1, typename SR2>
  void checkEquality(const SR1 &LHS, const SR2 &RHS, IDESolverConfig /*Tag*/) {
    llvm::errs() << "IDE Equality Check\n";
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

/// --> Using IDESolverConfig

TEST_F(IterativeIDESolverTest, IDESolverTestBranch) {
  doAnalysis("control_flow/branch_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDESolverTestLoop) {
  doAnalysis("control_flow/loop_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call06) {
  // Logger::initializeStderrLogger(SeverityLevel::DEBUG, "IterativeIDESolver");
  doAnalysis("linear_constant/call_06_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call07) {
  doAnalysis("linear_constant/call_07_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call08) {
  doAnalysis("linear_constant/call_08_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call09) {
  doAnalysis("linear_constant/call_09_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call10) {
  doAnalysis("linear_constant/call_10_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call11) {
  doAnalysis("linear_constant/call_11_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_Call12) {
  doAnalysis("linear_constant/call_12_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_recursion_01) {
  doAnalysis("linear_constant/recursion_01_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_recursion_02) {
  doAnalysis("linear_constant/recursion_02_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_recursion_03) {
  doAnalysis("linear_constant/recursion_03_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_01) {
  doAnalysis("linear_constant/global_01_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_02) {
  doAnalysis("linear_constant/global_02_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_03) {
  doAnalysis("linear_constant/global_03_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_04) {
  doAnalysis("linear_constant/global_04_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_05) {
  doAnalysis("linear_constant/global_05_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_06) {
  doAnalysis("linear_constant/global_06_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_07) {
  doAnalysis("linear_constant/global_07_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_08) {
  doAnalysis("linear_constant/global_08_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_09) {
  doAnalysis("linear_constant/global_09_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_global_10) {
  doAnalysis("linear_constant/global_10_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IDELinearConstant_external_fun_01) {
  doAnalysis("linear_constant/external_fun_cpp.ll");
}

/// <-- Using IDESolverConfig

/// --> Using IFDSSolverConfig

TEST_F(IterativeIDESolverTest, IFDSSolverTestBranch) {
  doAnalysis<IFDSSolverConfig>("control_flow/branch_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSSolverTestLoop) {
  doAnalysis<IFDSSolverConfig>("control_flow/loop_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call06) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_06_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call07) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_07_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call08) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_08_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call09) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_09_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call10) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_10_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call11) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_11_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_Call12) {
  doAnalysis<IFDSSolverConfig>("linear_constant/call_12_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_recursion_01) {
  doAnalysis<IFDSSolverConfig>("linear_constant/recursion_01_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_recursion_02) {
  doAnalysis<IFDSSolverConfig>("linear_constant/recursion_02_cpp.ll");
}
TEST_F(IterativeIDESolverTest, IFDSLinearConstant_recursion_03) {
  doAnalysis<IFDSSolverConfig>("linear_constant/recursion_03_cpp.ll");
}

/// <-- Using IFDSSolverConfig

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
