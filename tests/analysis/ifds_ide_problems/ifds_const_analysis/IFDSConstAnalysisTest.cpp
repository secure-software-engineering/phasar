#include "../../../../src/analysis/ifds_ide/solver/LLVMIFDSSolver.h"
#include "../../../../src/analysis/ifds_ide_problems/ifds_const_analysis/IFDSConstAnalysis.h"
#include "../../../../src/config/Configuration.h"
#include "../../../../src/db/ProjectIRDB.h"
#include <gtest/gtest.h>
using namespace std;

/* ============== GROUND TRUTH FOR BASIC TESTS ============== */
const map<unsigned, set<string>> basic_01_result = {};

const map<unsigned, set<string>> basic_02_result = {
    {5, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> basic_03_result = {
    {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> basic_04_result = {
    {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> basic_05_result = {
    {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {13, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {14, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {15, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {16, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> basic_06_result = {
    {21, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

/* ============== GROUND TRUTH FOR CALL TESTS ============== */
const map<unsigned, set<string>> call_01_result = {
    {5, {"%1 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}}};

const map<unsigned, set<string>> call_02_result = {
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 4"}},
    {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 4"}}};

const map<unsigned, set<string>> call_03_result = {
    {5, {"%1 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}},
    {13, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 7"}},
    {14, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 7"}}};

const map<unsigned, set<string>> call_param_01_result = {
    {8, {"%3 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> call_param_02_result = {
    {5, {"%2 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}}};

const map<unsigned, set<string>> call_param_03_result = {
    {6,
     {"i32* %0",
      "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2"}},
    {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const map<unsigned, set<string>> call_param_04_result = {
    {6,
     {"i32* %0",
      "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2"}},
    {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const map<unsigned, set<string>> call_param_05_result = {
    {6,
     {"%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2",
      "i32* %0"}},
    {15,
     {"%4 = load i32*, i32** %3, align 8, !phasar.instruction.id !7, ID: 13",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const map<unsigned, set<string>> call_param_06_result = {
    {9,
     {"%6 = load i32*, i32** %3, align 8, !phasar.instruction.id !6, ID: 5",
      "i32* %0"}},
    {18, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 11"}}};

const map<unsigned, set<string>> call_ret_01_result = {
    {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 2"}}};

const map<unsigned, set<string>> call_ret_02_result = {
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 5"}}};

const map<unsigned, set<string>> call_ret_03_result = {
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 5"}}};

const map<unsigned, set<string>> call_ret_05_result = {};

/* ============== GROUND TRUTH FOR CONTROL FLOW TESTS ============== */
const map<unsigned, set<string>> cf_for_01_result = {
    {7,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {8,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {9,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {10,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {11,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {12,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {13,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {14,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {15,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {16,
     {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {17,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {18,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {19,
     {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
    {20,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {21,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const map<unsigned, set<string>> cf_if_01_result = {
    {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const map<unsigned, set<string>> cf_if_02_result = {
    {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {13, {"%3 = alloca i32, align 4, !phasar.instruction.id !4, ID: 3"}},
    {14,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "%3 = alloca i32, align 4, !phasar.instruction.id !4, ID: 3"}}};

const map<unsigned, set<string>> cf_while_01_result = {
    {7, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {8, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {9, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {10, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {11, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {12, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

/* ============== GROUND TRUTH FOR GLOBAL TESTS ============== */
const map<unsigned, set<string>> global_01_result = {};

const map<unsigned, set<string>> global_02_result = {
    {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {8,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {9,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {10,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "@gint = global i32 10, align 4, !phasar.instruction.id !0"}},
    {11,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {12,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
    {13,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "@gint = global i32 10, align 4, !phasar.instruction.id !0"}},
    {14,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const map<unsigned, set<string>> global_03_result = {
    {7,
     {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !6, ID: 5"}}};

const map<unsigned, set<string>> global_04_result = {
    {4, {"@gint = global i32 10, align 4, !phasar.instruction.id !0"}}};

const map<unsigned, set<string>> global_05_result = {
    {5,
     {"@gint = global i32* null, align 8, !phasar.instruction.id !0",
      "%1 = load i32*, i32** @gint, align 8, !phasar.instruction.id !2, ID: "
      "1"}},
    {11, {"@gint = global i32* null, align 8, !phasar.instruction.id !0"}},
    {12, {"@gint = global i32* null, align 8, !phasar.instruction.id !0"}}};

/* ============== GROUND TRUTH FOR POINTER TESTS ============== */
const map<unsigned, set<string>> pointer_01_result = {
    {8,
     {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%4 = load i32*, i32** %3, align 8, !phasar.instruction.id !7, ID: 6"}}};

const map<unsigned, set<string>> pointer_02_result = {
    {9, {"%4 = alloca i32*, align 8, !phasar.instruction.id !4, ID: 3"}}};

const map<unsigned, set<string>> pointer_03_result = {
    {11,
     {"%6 = load i32*, i32** %5, align 8, !phasar.instruction.id !10, ID: 9",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> pointer_04_result = {
    {11,
     {"%6 = load i32*, i32** %3, align 8, !phasar.instruction.id !10, ID: 9",
      "%5 = load i32*, i32** %3, align 8, !phasar.instruction.id !8, ID: 7",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const map<unsigned, set<string>> pointer_heap_01_result = {};

const map<unsigned, set<string>> pointer_heap_02_result = {};

const map<unsigned, set<string>> pointer_heap_03_result = {
    {9,
     {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
    {10,
     {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
    {11,
     {"%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4"}},
    {12,
     {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
    {13,
     {"%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12"}},
    {14,
     {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
    {15,
     {"%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4"}}};

const map<unsigned, set<string>> pointer_heap_04_result = {};

/* ============== STRUCTS TESTS ============== */
const map<unsigned, set<string>> structs_01_result = {};

const map<unsigned, set<string>> structs_02_result = {};



/* ============== TEST ENVIRONMENT AND FIXTURE ============== */
/* Global Environment */
class IFDSConstAnalysisEnv : public ::testing::Environment {
public:
  virtual ~IFDSConstAnalysisEnv() {}

  virtual void SetUp() {
    initializeLogger(true);
    string tests_config_file = "tests/test_params.conf";
    ifstream ifs(tests_config_file.c_str());
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
  const string pathToTests = "test_code/llvm_test_code/constness/";
  const vector<string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IFDSConstAnalysis *constproblem;
  //  LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&>
  //    *llvmconstsolver;

  IFDSConstAnalysisTest() {}
  virtual ~IFDSConstAnalysisTest() {}

  void SetUp(const vector<string> &IRFiles) {
    ValueAnnotationPass::resetValueID();
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG = new LLVMBasedICFG(*TH, *IRDB, WalkerStrategy::Pointer,
                             ResolveStrategy::OTF, EntryPoints);
    constproblem = new IFDSConstAnalysis(*ICFG, EntryPoints);
    //    llvmconstsolver = new LLVMIFDSSolver<const llvm::Value*,
    //    LLVMBasedICFG&>(*constproblem, true);
  }

  virtual void TearDown() {
    delete IRDB;
    delete TH;
    delete ICFG;
    delete constproblem;
    //    delete llvmconstsolver;
  }

  void compareResultSets(
      const map<unsigned, set<string>> &groundTruth,
      LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> &solver) {
    for (auto F : IRDB->getAllFunctions()) {
      for (auto &BB : *F) {
        for (auto &I : BB) {
          int InstID = stoi(getMetaDataID(&I));
          set<const llvm::Value *> results = solver.ifdsResultsAt(&I);
          set<string> str_results;
          for (auto D : results) {
            // we ignore the zero value for convenience
            if (!constproblem->isZeroValue(D)) {
              string d_str = constproblem->DtoString(D);
              boost::trim_left(d_str);
              str_results.insert(d_str);
            }
          }
          if (!str_results.empty()) {
            cout << "IID:" << InstID << endl;
            for (auto s : str_results) {
              cout << s << endl;
            }
            cout << endl;
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
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_02) {
  SetUp({pathToTests + "basic/basic_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_03) {
  SetUp({pathToTests + "basic/basic_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_04) {
  SetUp({pathToTests + "basic/basic_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_05) {
  SetUp({pathToTests + "basic/basic_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_05_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleBasicTest_06) {
  SetUp({pathToTests + "basic/basic_06.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(basic_06_result, llvmconstsolver);
}

/* ============== CALL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleCallTest_01) {
  SetUp({pathToTests + "call/call_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallTest_02) {
  SetUp({pathToTests + "call/call_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallTest_03) {
  SetUp({pathToTests + "call/call_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_01) {
  SetUp({pathToTests + "call/param/call_param_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_param_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_02) {
  SetUp({pathToTests + "call/param/call_param_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_param_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_03) {
  SetUp({pathToTests + "call/param/call_param_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_param_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_04) {
  SetUp({pathToTests + "call/param/call_param_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_param_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallParamTest_05) {
  SetUp({pathToTests + "call/param/call_param_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_param_05_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_01) {
  SetUp({pathToTests + "call/return/call_ret_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_ret_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_02) {
  SetUp({pathToTests + "call/return/call_ret_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_ret_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_03) {
  SetUp({pathToTests + "call/return/call_ret_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_ret_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_04) {
  SetUp({pathToTests + "call/return/call_ret_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_ret_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleCallReturnTest_05) {
  SetUp({pathToTests + "call/return/call_ret_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(call_ret_05_result, llvmconstsolver);
}

/* ============== CONTROL FLOW TESTS ============== */

/* ============== GLOBAL TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_01) {
  SetUp({pathToTests + "global/global_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(global_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_02) {
  SetUp({pathToTests + "global/global_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(global_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_03) {
  SetUp({pathToTests + "global/global_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(global_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_04) {
  SetUp({pathToTests + "global/global_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(global_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandleGlobalTest_05) {
  SetUp({pathToTests + "global/global_05.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(global_05_result, llvmconstsolver);
}


/* ============== POINTER TESTS ============== */
TEST_F(IFDSConstAnalysisTest, HandlePointerTest_01) {
  SetUp({pathToTests + "pointer/pointer_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_02) {
  SetUp({pathToTests + "pointer/pointer_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_03) {
  SetUp({pathToTests + "pointer/pointer_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerTest_04) {
  SetUp({pathToTests + "pointer/pointer_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
      *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_04_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_01) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_01.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_01_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_02) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_02.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_02_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_03) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_03.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_03_result, llvmconstsolver);
}

TEST_F(IFDSConstAnalysisTest, HandlePointerHeapTest_04) {
  SetUp({pathToTests + "pointer/heap/pointer_heap_04.ll"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmconstsolver(
    *constproblem, true);
  llvmconstsolver.solve();
  compareResultSets(pointer_heap_04_result, llvmconstsolver);
}

/* ============== STRUCTS TESTS ============== */

/* ============== OTHER TESTS ============== */

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new IFDSConstAnalysisEnv());
  return RUN_ALL_TESTS();
}
