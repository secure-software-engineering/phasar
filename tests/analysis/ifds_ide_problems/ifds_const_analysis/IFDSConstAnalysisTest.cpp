#include "../../../../src/analysis/ifds_ide/solver/LLVMIFDSSolver.h"
#include "../../../../src/analysis/ifds_ide_problems/ifds_const_analysis/IFDSConstAnalysis.h"
#include "../../../../src/config/Configuration.h"
#include "../../../../src/db/ProjectIRDB.h"
#include "IFDSConstAnalysisResults.h"
#include <gtest/gtest.h>

/* ============== TEST ENVIRONMENT AND FIXTURE ============== */
/* Global Environment */
class IFDSConstAnalysisEnv : public ::testing::Environment {
public:
  virtual ~IFDSConstAnalysisEnv() {}

  virtual void SetUp() {
    initializeLogger(false);
    std::string tests_config_file = "tests/test_params.conf";
    std::ifstream ifs(tests_config_file.c_str());
    ASSERT_TRUE(ifs);
    bpo::options_description test_desc("Testing options");
    // clang-format off
    test_desc.add_options()
      ("mem2reg,M", bpo::value<bool>()->default_value(0), "Promote memory to register pass (1 or 0)");
    ;
    // clang-format on
    bpo::store(bpo::parse_config_file(ifs, test_desc), VariablesMap);
    bpo::notify(VariablesMap);
  }

  virtual void TearDown() {}
};

/* Test fixture */
class IFDSConstAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToTests = "test_code/llvm_test_code/constness/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IFDSConstAnalysis *constproblem;

  IFDSConstAnalysisTest() {}
  virtual ~IFDSConstAnalysisTest() {}

  void SetUp(const std::vector<std::string> &IRFiles) {
    ValueAnnotationPass::resetValueID();
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG = new LLVMBasedICFG(*TH, *IRDB, WalkerStrategy::Pointer,
                             ResolveStrategy::OTF, EntryPoints);
    constproblem = new IFDSConstAnalysis(*ICFG, EntryPoints);
  }

  virtual void TearDown() {
    PAMM_FACTORY;
    delete IRDB;
    delete TH;
    delete ICFG;
    delete constproblem;
    PAMM_RESET;
  }

  void compareResultSets(
      const std::map<unsigned, std::set<std::string>> &groundTruth,
      LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> &solver) {
    for (auto F : IRDB->getAllFunctions()) {
      for (auto &BB : *F) {
        for (auto &I : BB) {
          int InstID = stoi(getMetaDataID(&I));
          std::set<const llvm::Value *> results = solver.ifdsResultsAt(&I);
          std::set<std::string> str_results;
          for (auto D : results) {
            // we ignore the zero value for convenience
            if (!constproblem->isZeroValue(D)) {
              std::string d_str = constproblem->DtoString(D);
              boost::trim_left(d_str);
              str_results.insert(d_str);
            }
          }
          if (!str_results.empty()) {
            std::cout << "IID:" << InstID << std::endl;
            for (auto s : str_results) {
              std::cout << s << std::endl;
            }
            std::cout << std::endl;
            auto it = groundTruth.find(InstID);
            if (it != groundTruth.end()) {
              EXPECT_EQ(str_results, it->second);
            } else {
              // Instruction Id not listed in the results
              ASSERT_NE(it, groundTruth.end());
            }
          }
        }
      }
    }
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleBasicTest_01) {
  SetUp({pathToTests + "basic/basic_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  SetUp({pathToTests + "basic/basic_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  SetUp({pathToTests + "basic/basic_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  SetUp({pathToTests + "basic/basic_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_05) {
  SetUp({pathToTests + "basic/basic_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_05_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_06) {
  SetUp({pathToTests + "basic/basic_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(basic_06_result, llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallTest_01) {
  SetUp({pathToTests + "call/call_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallTest_02) {
  SetUp({pathToTests + "call/call_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallTest_03) {
  SetUp({pathToTests + "call/call_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  SetUp({pathToTests + "call/param/call_param_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  SetUp({pathToTests + "call/param/call_param_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  SetUp({pathToTests + "call/param/call_param_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_04) {
  SetUp({pathToTests + "call/param/call_param_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_05) {
  SetUp({pathToTests + "call/param/call_param_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_05_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_06) {
  SetUp({pathToTests + "call/param/call_param_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_param_06_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  SetUp({pathToTests + "call/return/call_ret_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_ret_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  SetUp({pathToTests + "call/return/call_ret_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_ret_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  SetUp({pathToTests + "call/return/call_ret_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_ret_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCallReturnTest_04) {
  SetUp({pathToTests + "call/return/call_ret_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(call_ret_04_result, llvmconstsolver);
}


/* ============== CONTROL FLOW TESTS ============== */

TEST_F(IFDSConstAnalysisTest, HandleCFForTest_01) {
  SetUp({pathToTests + "control_flow/cf_for_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(cf_for_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, DISABLED_HandleCFForTest_02) {
  SetUp({pathToTests + "control_flow/cf_for_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(cf_for_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_01) {
  SetUp({pathToTests + "control_flow/cf_if_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(cf_if_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFIfTest_02) {
  SetUp({pathToTests + "control_flow/cf_if_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(cf_if_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCFWhileTest_01) {
  SetUp({pathToTests + "control_flow/cf_while_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(cf_while_01_result, llvmconstsolver);
}

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  SetUp({pathToTests + "global/global_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(global_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  SetUp({pathToTests + "global/global_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(global_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  SetUp({pathToTests + "global/global_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(global_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_04) {
  SetUp({pathToTests + "global/global_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(global_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_05) {
  SetUp({pathToTests + "global/global_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(global_05_result, llvmconstsolver);
}

/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  SetUp({pathToTests + "pointer/pointer_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  SetUp({pathToTests + "pointer/pointer_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_03) {
  SetUp({pathToTests + "pointer/pointer_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  SetUp({pathToTests + "pointer/pointer_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_01) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_02) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_03) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_04) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_04_result, llvmconstsolver);
}

/* ============== STRUCTS TESTS ============== */

TEST_F(IFDSConstAnalysisTest, HandleStructsTest_01) {
  SetUp({pathToTests + "structs/structs_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(structs_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleStructsTest_02) {
  SetUp({pathToTests + "structs/structs_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, false);
  llvmconstsolver.solve();
  compareResultSets(structs_02_result, llvmconstsolver);
}

/* ============== OTHER TESTS ============== */

// TODO: Add missing tests

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new IFDSConstAnalysisEnv());
  return RUN_ALL_TESTS();
}
