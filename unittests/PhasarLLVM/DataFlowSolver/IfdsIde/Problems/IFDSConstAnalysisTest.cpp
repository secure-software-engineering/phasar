#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSConstAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/constness/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  IFDSConstAnalysis *constproblem;

  IFDSConstAnalysisTest() {}
  virtual ~IFDSConstAnalysisTest() {}

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);
    constproblem = new IFDSConstAnalysis(IRDB, TH, ICFG, PT, EntryPoints);
  }

  void SetUp() override {
    initializeLogger(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete PT;
    delete ICFG;
    delete constproblem;
  }

  void compareResults(
      const std::set<unsigned long> &groundTruth,
      IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
                 IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
                 IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t> &solver) {
    std::set<const llvm::Value *> allMutableAllocas;
    for (auto RR : IRDB->getRetOrResInstructions()) {
      std::set<const llvm::Value *> facts = solver.ifdsResultsAt(RR);
      for (auto fact : facts) {
        if (isAllocaInstOrHeapAllocaFunction(fact) ||
            (llvm::isa<llvm::GlobalValue>(fact) &&
             !constproblem->isZeroValue(fact))) {
          allMutableAllocas.insert(fact);
        }
      }
    }
    std::set<unsigned long> mutableIDs;
    for (auto memloc : allMutableAllocas) {
      mutableIDs.insert(std::stoul(getMetaDataID(memloc)));
    }
    EXPECT_EQ(groundTruth, mutableIDs);
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleBasicTest_01) {
  Initialize({pathToLLFiles + "basic/basic_01_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  Initialize({pathToLLFiles + "basic/basic_02_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  Initialize({pathToLLFiles + "basic/basic_03_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  Initialize({pathToLLFiles + "basic/basic_04_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

/* ============== CONTROL FLOW TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCFForTest_01) {
  Initialize({pathToLLFiles + "control_flow/cf_for_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFForTest_02) {
  Initialize({pathToLLFiles + "control_flow/cf_for_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_01) {
  Initialize({pathToLLFiles + "control_flow/cf_if_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_02) {
  Initialize({pathToLLFiles + "control_flow/cf_if_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFWhileTest_01) {
  Initialize({pathToLLFiles + "control_flow/cf_while_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  Initialize({pathToLLFiles + "pointer/pointer_01_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  Initialize({pathToLLFiles + "pointer/pointer_02_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandlePointerTest_03) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "pointer/pointer_03_cpp_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({2, 3}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  Initialize({pathToLLFiles + "pointer/pointer_04_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({4}, llvmconstsolver);
}

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  Initialize({pathToLLFiles + "global/global_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  Initialize({pathToLLFiles + "global/global_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0, 1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  Initialize({pathToLLFiles + "global/global_03_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleGlobalTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "global/global_04_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0, 4}, llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  Initialize({pathToLLFiles + "call/param/call_param_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({5}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  Initialize({pathToLLFiles + "call/param/call_param_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({5}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  Initialize({pathToLLFiles + "call/param/call_param_03_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "call/param/call_param_04_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallParamTest_05) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "call/param/call_param_05_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_06) {
  Initialize({pathToLLFiles + "call/param/call_param_06_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_07) {
  Initialize({pathToLLFiles + "call/param/call_param_07_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({6}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_08) {
  Initialize({pathToLLFiles + "call/param/call_param_08_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({4}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  Initialize({pathToLLFiles + "call/return/call_ret_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  Initialize({pathToLLFiles + "call/return/call_ret_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  Initialize({pathToLLFiles + "call/return/call_ret_03_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

/* ============== ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleArrayTest_01) {
  Initialize({pathToLLFiles + "array/array_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_02) {
  Initialize({pathToLLFiles + "array/array_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_03) {
  Initialize({pathToLLFiles + "array/array_03_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "array/array_04_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_05) {
  Initialize({pathToLLFiles + "array/array_05_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_06) {
  Initialize({pathToLLFiles + "array/array_06_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleArrayTest_07) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "array/array_07_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_08) {
  Initialize({pathToLLFiles + "array/array_08_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleArrayTest_09) {
  Initialize({pathToLLFiles + "array/array_09_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({0}, llvmconstsolver);
}

/* ============== STL ARRAY TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_01) {
  Initialize({pathToLLFiles + "array/stl_array/stl_array_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_02) {
  Initialize({pathToLLFiles + "array/stl_array/stl_array_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({1}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_03) {
  Initialize({pathToLLFiles + "array/stl_array/stl_array_03_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleSTLArrayTest_04) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "array/stl_array/stl_array_04_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_05) {
  Initialize({pathToLLFiles + "array/stl_array/stl_array_05_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleSTLArrayTest_06) {
  Initialize({pathToLLFiles + "array/stl_array/stl_array_06_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

/* ============== CSTRING TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCStringTest_01) {
  Initialize({pathToLLFiles + "array/cstring/cstring_01_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({}, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCStringTest_02) {
  // Guaranteed to fail - enable, once we have more precise points-to
  // information
  Initialize({pathToLLFiles + "array/cstring/cstring_02_cpp_m2r_dbg.ll"});
  IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
             IFDSConstAnalysis::f_t, IFDSConstAnalysis::t_t,
             IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>
      llvmconstsolver(*constproblem);
  llvmconstsolver.solve();
  compareResults({2}, llvmconstsolver);
}

/* ============== STRUCTURE TESTS ============== */
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_01) {
//  Initialize({pathToLLFiles + "structs/structs_01_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_02) {
//  Initialize({pathToLLFiles + "structs/structs_02_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 5}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_03) {
//  Initialize({pathToLLFiles + "structs/structs_03_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 9}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_04) {
//  Initialize({pathToLLFiles + "structs/structs_04_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1, 10}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_05) {
//  Initialize({pathToLLFiles + "structs/structs_05_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0, 1}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_06) {
//  Initialize({pathToLLFiles + "structs/structs_06_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_07) {
//  Initialize({pathToLLFiles + "structs/structs_07_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_08) {
//  Initialize({pathToLLFiles + "structs/structs_08_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_09) {
//  Initialize({pathToLLFiles + "structs/structs_09_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_10) {
//  Initialize({pathToLLFiles + "structs/structs_10_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_11) {
//  Initialize({pathToLLFiles + "structs/structs_11_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}
//
// TEST_F(IFDSConstAnalysisTest, HandleStructureTest_12) {
//  Initialize({pathToLLFiles + "structs/structs_12_cpp_dbg.ll"});
//  IFDSSolver<IFDSConstAnalysis::n_t,IFDSConstAnalysis::d_t,IFDSConstAnalysis::f_t,IFDSConstAnalysis::t_t,IFDSConstAnalysis::v_t,IFDSConstAnalysis::i_t>
//  llvmconstsolver(
//      *constproblem);
//  llvmconstsolver.solve();
//  compareResults({0}, llvmconstsolver);
//}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
