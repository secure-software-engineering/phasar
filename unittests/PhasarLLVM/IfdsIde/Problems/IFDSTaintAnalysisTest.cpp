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
    PAMM_FACTORY;
    delete IRDB;
    delete TH;
    delete ICFG;
    delete TaintProblem;
    delete TSF;
    PAMM_RESET;
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

TEST_F(IFDSTaintAnalysisTest, HandleControlFlow) {
  Initialize({pathToLLFiles + "../control_flow/function_call_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_01_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[10] = set<string>{"9"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_01_cpp_m2r.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[1] = set<string>{"0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_02_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[7] = set<string>{"6"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_03_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[14] = set<string>{"13"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_04_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[14] = set<string>{"13"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_05_cpp.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[17] = set<string>{"16"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  Initialize({pathToLLFiles + "dummy_source_sink/taint_06_cpp_m2r.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(
      *TaintProblem, false);
  TaintSolver.solve();
  TaintProblem->printLeaks();
  map<int, set<string>> GroundTruth;
  GroundTruth[2] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
