#include <memory>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
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
  const std::string PathToLlFiles = "llvm_test_code/taint_analysis/";
  const std::set<std::string> EntryPoints = {"main"};

  unique_ptr<ProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMBasedICFG> ICFG;
  unique_ptr<LLVMPointsToInfo> PT;
  unique_ptr<IFDSTaintAnalysis> TaintProblem;
  unique_ptr<TaintConfig> TSF;

  IFDSTaintAnalysisTest() = default;
  ~IFDSTaintAnalysisTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMPointsToSet>(*IRDB);
    ICFG = make_unique<LLVMBasedICFG>(
        IRDB.get(), CallGraphAnalysisType::OTF,
        std::vector<std::string>{EntryPoints.begin(), EntryPoints.end()},
        TH.get(), PT.get());
    TaintConfig::TaintDescriptionCallBackTy SourceCB =
        [](const llvm::Instruction *Inst) {
          std::set<const llvm::Value *> Ret;
          if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
              Call && Call->getCalledFunction() &&
              Call->getCalledFunction()->getName() == "_Z6sourcev") {
            Ret.insert(Call);
          }
          return Ret;
        };
    TaintConfig::TaintDescriptionCallBackTy SinkCB =
        [](const llvm::Instruction *Inst) {
          std::set<const llvm::Value *> Ret;
          if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
              Call && Call->getCalledFunction() &&
              Call->getCalledFunction()->getName() == "_Z4sinki") {
            assert(Call->arg_size() > 0);
            Ret.insert(Call->getArgOperand(0));
          }
          return Ret;
        };
    TSF = make_unique<TaintConfig>(std::move(SourceCB), std::move(SinkCB));
    TaintProblem = make_unique<IFDSTaintAnalysis>(
        IRDB.get(), TH.get(), ICFG.get(), PT.get(), *TSF, EntryPoints);
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
  initialize({PathToLlFiles + "dummy_source_sink/taint_01.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[13] = set<string>{"12"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_01_m2r) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_01.m2r.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[4] = set<string>{"2"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_02) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_02.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[9] = set<string>{"8"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_03) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_03.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[18] = set<string>{"17"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_04) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_04.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[19] = set<string>{"18"};
  GroundTruth[24] = set<string>{"23"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_05) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_05.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[22] = set<string>{"21"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_06) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_06.m2r.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[5] = set<string>{"main.0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_01.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_01_m2r) {
  initialize(
      {PathToLlFiles + "dummy_source_sink/taint_exception_01.m2r.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[6] = set<string>{"0"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_02) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_02.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[17] = set<string>{"16"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_03) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_03.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[11] = set<string>{"10"};
  GroundTruth[21] = set<string>{"20"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_04) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_04.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_05) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_05.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();

  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_06) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_06.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[15] = set<string>{"14"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_07) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_07.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[31] = set<string>{"30"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_08) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_08.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[33] = set<string>{"32"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_09) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_09.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[64] = set<string>{"63"};
  compareResults(GroundTruth);
}

TEST_F(IFDSTaintAnalysisTest, TaintTest_ExceptionHandling_10) {
  initialize({PathToLlFiles + "dummy_source_sink/taint_exception_10.dbg.ll"});
  IFDSSolver_P<IFDSTaintAnalysis> TaintSolver(*TaintProblem);
  TaintSolver.solve();
  map<int, set<string>> GroundTruth;
  GroundTruth[62] = set<string>{"61"};
  compareResults(GroundTruth);
}
