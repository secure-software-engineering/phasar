#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
//#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class IFDSUninitializedVariablesTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/uninitialized_variables/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IFDSUninitializedVariables *UninitProblem;

  IFDSUninitializedVariablesTest() {}
  virtual ~IFDSUninitializedVariablesTest() {}

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG =
        new LLVMBasedICFG(*TH, *IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    // TSF = new TaintSensitiveFunctions(true);
    UninitProblem =
        new IFDSUninitializedVariables(*ICFG, *TH, *IRDB, EntryPoints);
  }

  void SetUp() override {
    bl::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete ICFG;
    delete UninitProblem;
  }

  void compareResults(map<int, set<string>> &GroundTruth) {

    map<int, set<string>> FoundUninitUses;
    for (auto kvp : UninitProblem->getAllUndefUses()) {
      auto InstID = stoi(getMetaDataID(kvp.first));
      set<string> UndefValueIds;
      for (auto UV : kvp.second) {
        UndefValueIds.insert(getMetaDataID(UV));
      }
      FoundUninitUses[InstID] = UndefValueIds;
    }

    EXPECT_EQ(FoundUninitUses, GroundTruth);
  }
}; // Test Fixture

TEST_F(IFDSUninitializedVariablesTest, UninitTest_01_SHOULD_NOT_LEAK) {
  Initialize({pathToLLFiles + "all_uninit_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();
  // all_uninit.cpp does not contain undef-uses
  map<int, set<string>> GroundTruth;
  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_02_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "binop_uninit_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();

  // binop_uninit uses uninitialized variable i in 'int j = i + 10;'
  map<int, set<string>> GroundTruth;
  // %4 = load i32, i32* %2, ID: 6 ;  %2 is the uninitialized variable i
  GroundTruth[6] = {"1"};
  // %5 = add nsw i32 %4, 10 ;        %4 is undef, since it is loaded from
  // undefined alloca; not sure if it is necessary to report again
  GroundTruth[7] = {"6"};
  compareResults(GroundTruth);
}
TEST_F(IFDSUninitializedVariablesTest, UninitTest_03_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "callnoret_c_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();

  // callnoret uses uninitialized variable a in 'return a + 10;' of addTen(int)
  map<int, set<string>> GroundTruth;

  // %4 = load i32, i32* %2 ; %2 is the parameter a of addTen(int) containing
  // undef
  GroundTruth[5] = {"0"};
  // The same as in test2: is it necessary to report again?
  GroundTruth[6] = {"5"};
  // %5 = load i32, i32* %2 ; %2 is the uninitialized variable a
  GroundTruth[16] = {"9"};
  // The same as in test2: is it necessary to report again? (the analysis does
  // not)
  GroundTruth[17] = {"16"};

  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_04_SHOULD_NOT_LEAK) {
  Initialize({pathToLLFiles + "ctor_default_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();
  // ctor.cpp does not contain undef-uses
  map<int, set<string>> GroundTruth;
  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_05_SHOULD_NOT_LEAK) {
  Initialize({pathToLLFiles + "struct_member_init_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();
  // struct_member_init.cpp does not contain undef-uses
  map<int, set<string>> GroundTruth;
  compareResults(GroundTruth);
}
TEST_F(IFDSUninitializedVariablesTest, UninitTest_06_SHOULD_NOT_LEAK) {
  Initialize({pathToLLFiles + "struct_member_uninit_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              false, false);
  Solver.solve();
  // struct_member_uninit.cpp does not contain undef-uses
  map<int, set<string>> GroundTruth;
  compareResults(GroundTruth);
}

TEST_F(IFDSUninitializedVariablesTest, UninitTest_07_SHOULD_LEAK) {
  Initialize({pathToLLFiles + "struct_member_uninit2_cpp_dbg.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> Solver(*UninitProblem,
                                                              true, true);
  Solver.solve();
  // struct_member_uninit2.cpp contains a use of the uninitialized field _x.b
  map<int, set<string>> GroundTruth;
  // %5 = load i16, i16* %4; %4 is the uninitialized struct-member _x.b
  GroundTruth[4] = {"3"};

  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}