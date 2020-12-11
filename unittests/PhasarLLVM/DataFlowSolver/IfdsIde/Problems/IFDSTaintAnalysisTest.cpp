#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "gtest/gtest.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      unittest::PathToLLTestFiles + "taint_analysis/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB{};
  LLVMTypeHierarchy *TH{};
  LLVMBasedICFG *ICFG{};
  LLVMPointsToInfo *PT{};
  IFDSTaintAnalysis *TaintProblem{};
  TaintConfiguration<const llvm::Value *> *TSF{};

  IFDSTaintAnalysisTest() = default;
  ~IFDSTaintAnalysisTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToSet(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);
    TSF = new TaintConfiguration<const llvm::Value *>(
        {TaintConfiguration<const llvm::Value *>::SourceFunction("source()",
                                                                 true)},
        {TaintConfiguration<const llvm::Value *>::SinkFunction(
            "sink(int)", std::vector<unsigned>({0}))});
    TaintProblem = new IFDSTaintAnalysis(IRDB, TH, ICFG, PT, *TSF, EntryPoints);
  }

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
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
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[13] = set<string>{"12"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[4] = set<string>{"2"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_02_cpp_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[9] = set<string>{"8"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_03_cpp_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[18] = set<string>{"17"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_04_cpp_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[19] = set<string>{"18"};
  GroundTruth[24] = set<string>{"23"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_05_cpp_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[22] = set<string>{"21"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_06_cpp_m2r_dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
