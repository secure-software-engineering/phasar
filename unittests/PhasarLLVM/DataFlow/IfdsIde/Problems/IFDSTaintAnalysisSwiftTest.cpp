#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSTaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SWIFT_SUBFOLDER("taint_analysis/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<IFDSTaintAnalysis> TaintProblem;
  std::optional<LLVMTaintConfig> TSF;

  IFDSTaintAnalysisTest() = default;
  ~IFDSTaintAnalysisTest() override = default;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);
    LLVMTaintConfig::TaintDescriptionCallBackTy SourceCB =
        [](const llvm::Instruction *Inst) {
          std::set<const llvm::Value *> Ret;
          if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
              Call && Call->getCalledFunction() &&
              // Swift provides CLI arguments through a method call to
              // this method to the program.
              // TODO: handle this in the analysis, like this we need
              // to include the method explicitly in every taint config
              // which is not ideal
              (Call->getCalledFunction()->getName() == "source" ||
               Call->getCalledFunction()->getName().contains_insensitive(
                   "MyMainV6sourceSi") ||
               Call->getCalledFunction()->getName().equals(
                   "$ss11CommandLineO9argumentsSaySSGvgZ"))) {
            Ret.insert(Call);
          }
          return Ret;
        };
    LLVMTaintConfig::TaintDescriptionCallBackTy SinkCB =
        [](const llvm::Instruction *Inst) {
          std::set<const llvm::Value *> Ret;
          if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
              Call && Call->getCalledFunction() &&
              // Swift generates different function names whether a function is
              // externaly declared or actually present in the code
              (Call->getCalledFunction()->getName() == "sink" ||
               Call->getCalledFunction()->getName().contains_insensitive(
                   "MyMainV4sinkyySiFZ"))) {
            assert(Call->arg_size() > 0);
            Ret.insert(Call->getArgOperand(0));
          }
          return Ret;
        };
    TSF.emplace(std::move(SourceCB), std::move(SinkCB));
    TaintProblem =
        createAnalysisProblem<IFDSTaintAnalysis>(*HA, &*TSF, EntryPoints);
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  void compareResults(map<int, set<string>> &GroundTruth) {
    // std::map<n_t, std::set<d_t>> Leaks;
    map<int, set<string>> FoundLeaks;
    for (const auto &Leak : TaintProblem->Leaks) {
      int SinkId = stoi(getMetaDataID(Leak.first));
      set<string> LeakedValueIds;
      for (const auto *LV : Leak.second) {
        LeakedValueIds.insert(getMetaDataID(LV));
      }
      FoundLeaks.insert(make_pair(SinkId, LeakedValueIds));
    }
    EXPECT_EQ(FoundLeaks, GroundTruth);
  }
}; // Test Fixture

TEST_F(IFDSTaintAnalysisTest, TaintTest_01) {
  initialize({PathToLlFiles + "taint_01.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  // sink --> taint
  GroundTruth[23] = set<string>{"20"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  GTEST_SKIP() << "Swift taint tests with CLI arguments are not supported yet";
  initialize({PathToLlFiles + "taint_02.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {

  initialize({PathToLlFiles + "taint_03.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[91] = set<string>{"88"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  initialize({PathToLlFiles + "taint_04.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  // This test case is a bit different than the C++ one
  // the assignemt a = b is ignored in Swift-based
  // LLVM IR and thus the same value is tainted for
  // both sink calls
  GroundTruth[95] = set<string>{"92"};
  GroundTruth[100] = set<string>{"92"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  initialize({PathToLlFiles + "taint_05.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[42] = set<string>{"39"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  GTEST_SKIP() << "Swift taint tests with CLI arguments are not supported yet";
  initialize({PathToLlFiles + "taint_06.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_04) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_04.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[77] = set<string>{"72"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_05) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_05.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();

  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_06) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_06.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_07) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_07.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[31] = set<string>{"30"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_08) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_08.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_09) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_09.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[64] = set<string>{"63"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_10) {
  GTEST_SKIP() << "Swift taint tests with Exceptions are not supported yet";
  initialize({PathToLlFiles + "taint_exception_10.ll"});
  IFDSSolver TaintSolver(*TaintProblem, &HA->getICFG());
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[62] = set<string>{"61"};
  compareResults(GroundTruth);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
