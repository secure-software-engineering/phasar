#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/taint_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IFDSTaintAnalysis *TaintProblem;
  TaintSensitiveFunctions *TSF;

  IFDSTaintAnalysisTest() {}
  virtual ~IFDSTaintAnalysisTest() {}

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG =
        new LLVMBasedICFG(*TH, *IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    TSF = new TaintSensitiveFunctions(true);
    TaintProblem = new IFDSTaintAnalysis(*ICFG, *TSF, EntryPoints);
  }

  void SetUp() override {
    bl::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete ICFG;
    delete TaintProblem;
    delete TSF;
  }

  void compareResults(map<int, set<string>> &GroundTruth) {
    // std::map<n_t, std::set<d_t>> Leaks;
    map<int, set<string>> FoundLeaks;
    for (auto Leak : TaintProblem->Leaks) {
      int SinkId = stoi(getMetaDataID(Leak.first));
      set<string> LeakedValueIds;
      for (auto LV : Leak.second) {
        LeakedValueIds.insert(getMetaDataID(LV));
      }
      FoundLeaks.insert(make_pair(SinkId, LeakedValueIds));
    }
    EXPECT_EQ(FoundLeaks, GroundTruth);
  }
}; // Test Fixture

TEST_F(IFDSTaintAnalysisTest, TaintTest_01) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_01_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[13] = set<string>{"12"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_01_cpp_m2r_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[4] = set<string>{"2"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_02_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[9] = set<string>{"8"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_03_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[18] = set<string>{"17"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_04_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[19] = set<string>{"18"};
  GroundTruth[24] = set<string>{"23"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_05_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[22] = set<string>{"21"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_06_cpp_m2r_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false, true);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
