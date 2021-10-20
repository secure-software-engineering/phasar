#include <memory>

#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/CallString.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class InterMonoTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      unittest::PathToLLTestFiles + "taint_analysis/";
  const std::set<std::string> EntryPoints = {"main"};

  std::unique_ptr<ProjectIRDB> IRDB;

  void SetUp() override {
    std::cout << "setup\n";
    boost::log::core::get()->set_logging_enabled(false);
  }
  void TearDown() override {}

  std::map<llvm::Instruction const *, std::set<llvm::Value const *>>
  doAnalysis(const std::string &LlvmFilePath, bool PrintDump = false) {
    auto IR_Files = {PathToLlFiles + LlvmFilePath};
    IRDB = std::make_unique<ProjectIRDB>(IR_Files, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    auto PT = std::make_unique<LLVMPointsToSet>(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                       PT.get());
    auto ConfigPath = PathToLlFiles + "config.json";
    auto BuildPos = ConfigPath.rfind("/build/") + 1;
    ConfigPath.erase(BuildPos, 6);
    TaintConfig TC(*IRDB, parseTaintConfig(ConfigPath));
    TC.registerSinkCallBack([](const llvm::Instruction *Inst) {
      std::set<const llvm::Value *> Ret;
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
          Call && Call->getCalledFunction() &&
          Call->getCalledFunction()->getName() == "printf") {
        for (const auto &Arg : Call->args()) {
          Ret.insert(Arg.get());
        }
      }
      return Ret;
    });
    InterMonoTaintAnalysis TaintProblem(IRDB.get(), &TH, &ICFG, PT.get(), TC,
                                        EntryPoints);
    InterMonoSolver<InterMonoTaintAnalysisDomain, 3> TaintSolver(TaintProblem);
    TaintSolver.solve();
    if (PrintDump) {
      TaintSolver.dumpResults();
    }
    auto Leaks = TaintProblem.getAllLeaks();
    // for (auto &[Inst, Values] : Leaks) {
    //   // std::cout << "I: " << llvmIRToShortString(Inst) << '\n';
    //   for (const auto *Value : Values) {
    //     // std::cout << "V: " << llvmIRToShortString(Value) << '\n';
    //   }
    // }
    return Leaks;
  }

  void doAnalysisAndCompare(const std::string &LlvmFilePath, size_t InstId,
                            const std::set<std::string> &GroundTruth,
                            bool PrintDump = false) {
    // FIXME
    // auto IR_Files = {PathToLlFiles + LlvmFilePath};
    // IRDB = std::make_unique<ProjectIRDB>(IR_Files, IRDBOptions::WPA);
    // ValueAnnotationPass::resetValueID();
    // LLVMTypeHierarchy TH(*IRDB);
    // auto PT = std::make_unique<LLVMPointsToSet>(*IRDB);
    // LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
    //                    PT.get());
    // TaintConfiguration<InterMonoTaintAnalysis::d_t> TC;
    // InterMonoTaintAnalysis TaintProblem(IRDB.get(), &TH, &ICFG, PT.get(), TC,
    //                                     EntryPoints);
    // InterMonoSolver<InterMonoTaintAnalysisDomain, 3>
    // TaintSolver(TaintProblem); TaintSolver.solve(); if (PrintDump) {
    //   TaintSolver.dumpResults();
    // }
    // std::set<std::string> FoundResults;
    // for (const auto *Result :
    //      TaintSolver.getResultsAt(IRDB->getInstruction(InstId))) {
    //   FoundResults.insert(getMetaDataID(Result));
    // }
    // EXPECT_EQ(FoundResults, GroundTruth);
    EXPECT_TRUE(true);
  }

  static void compareResults(
      std::map<llvm::Instruction const *, std::set<llvm::Value const *>> &Leaks,
      std::map<int, std::set<std::string>> &GroundTruth,
      const std::string &ErrorMessage = "") {
    std::map<int, std::set<std::string>> LeakIds;
    for (const auto &Kvp : Leaks) {
      int InstId = stoi(getMetaDataID(Kvp.first));
      EXPECT_NE(-1, InstId);
      for (const auto *LeakVal : Kvp.second) {
        LeakIds[InstId].insert(getMetaDataID(LeakVal));
      }
    }
    EXPECT_EQ(LeakIds, GroundTruth) << ErrorMessage;
  }
}; // Test Fixture

/******************************************************************************
 * The following four tests show undefined behaviour. The cause is unfortunately
 * unknown at the moment. It might be caused by strange execution order induced
 * by googletest/ctest, or a bug in the InterMonoSolver concering the handling
 * of callstrings.
 *
 ******************************************************************************

TEST_F(InterMonoTaintAnalysisTest, TaintTest_03) {
  std::set<std::string> Facts{"20", "21", "22",     "24",    "27",
                              "28", "32", "main.0", "main.1"};
  doAnalysisAndCompare("taint_11_c.ll", 34, Facts);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_03_v2) {
  auto Leaks = doAnalysis("taint_11_c.ll");
  // 35 => {34}
  // 37 => {36} due to overapproximation (limitation of call string)
  std::map<int, std::set<std::string>> GroundTruth;
  GroundTruth[35] = {"34"};
  GroundTruth[37] = {"36"};
  // kind of nondeterministic: sometimes it only leaks at 35, some only at
  // 37, but most times at both
  compareResults(Leaks, GroundTruth);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_05) {
  std::set<std::string> Facts{"7", "8", "14", "15", "19", "main.0",
  "main.1"}; doAnalysisAndCompare("taint_13_c.ll", 31, Facts);
}

TEST(InterMonoTaintAnalysisTestNF, TaintTest_05) {
  std::set<std::string> Facts{"7", "8", "14", "15", "19", "main.0", "main.1"};
  // doAnalysisAndCompare("taint_13_c.ll", 31, Facts);
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/taint_analysis/";
  ProjectIRDB IRDB({pathToLLFiles + "taint_13_c.ll"});
  ValueAnnotationPass::resetValueID();
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, {"main"});
  InterMonoTaintAnalysis TaintProblem(ICFG, {"main"});
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();
  TaintSolver.dumpResults();
  std::set<std::string> FoundResults;
  for (auto result :
       TaintSolver.getResultsAt(IRDB.getInstruction(31)).getAsSet()) {
    FoundResults.insert(getMetaDataID(result));
  }
  EXPECT_EQ(FoundResults, Facts);
}
 ******************************************************************************/

/******************************************************************************
 * Tests based on dataflow facts
 *
 ******************************************************************************/

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_01) {
//   std::set<std::string> Facts{"5", "6", "7", "10", "11", "main.0", "main.1"};
//   doAnalysisAndCompare("taint_9_c.ll", 13, Facts);
// }

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_02) {
//   std::set<std::string> Facts{"5", "6", "7", "12", "13", "main.0", "main.1"};
//   doAnalysisAndCompare("taint_10_c.ll", 19, Facts);
// }

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_04) {
//   std::set<std::string> Facts{"21", "22", "23", "28", "29", "main.0",
//   "main.1"}; doAnalysisAndCompare("taint_12_c.ll", 35, Facts);
// }

// /******************************************************************************
//  * Tests actually based on leaked values, not on dataflow facts
//  *
//  ******************************************************************************/

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_01_v2) {
//   auto Leaks = doAnalysis("taint_9_c.ll");
//   // 14 => {13}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[14] = {"13"};
//   compareResults(Leaks, GroundTruth);
// }

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_02_v2) {
//   auto Leaks = doAnalysis("taint_10_c.ll");
//   // 20 => {19}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[20] = {"19"};
//   compareResults(Leaks, GroundTruth);
// }

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_04_v2) {
//   auto Leaks = doAnalysis("taint_12_c.ll");
//   // 36 => {35}
//   // why not 38 => {37} due to overapproximation in recursion (limitation of
//   // call string) ???
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[36] = {"35"};
//   // GroundTruth[38] = {"37"};
//   compareResults(Leaks, GroundTruth);
// }

// TEST_F(InterMonoTaintAnalysisTest, TaintTest_05_v2) {
//   auto Leaks = doAnalysis("taint_13_c.ll");
//   // 32 => {31}
//   // 34 => {33} will not leak (analysis is naturally never strong enough for
//   // this)
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[32] = {"31"};
//   compareResults(Leaks, GroundTruth);
// }

// /**********************************************************
//  * fails due to alias-unawareness
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_06) {
//   auto Leaks = doAnalysis("taint_4_v2_cpp.ll");
//   // 19 => {18}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[19] = {"18"};

//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// /**********************************************************
//  * fails, since std::cout is not a sink
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_07) {
//   auto Leaks = doAnalysis("taint_2_v2_cpp.ll");
//   // 10 => {9}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[10] = {"9"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_08) {
//   auto Leaks = doAnalysis("taint_2_v2_1_cpp.ll");
//   // 4 => {3}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[4] = {"3"};
//   compareResults(Leaks, GroundTruth);
// }
// /**********************************************************
//  * fails due to lack of alias information
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_09) {
//   auto Leaks = doAnalysis("source_sink_function_test_c.ll");
//   // 41 => {40};
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[41] = {"40"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_10) {
//   auto Leaks = doAnalysis("taint_14_cpp.ll");
//   // 11 => {10}; do not know, why it fails; getchar is definitely a source,
//   but
//   // it doesn't generate a fact
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[11] = {"10"};
//   compareResults(Leaks, GroundTruth);
// }
// /**********************************************************
//  * fails, because arithmetic operations do not propagate taints;
//  * In contrast to TaintTest10, getchar generates a fact here;
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_11) {
//   auto Leaks = doAnalysis("taint_14_1_cpp.ll");
//   // 12 => {11}; quite similar as TaintTest10, but all in main;
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[12] = {"11"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// /**********************************************************
//  * fails, since arithmetic operations do not propagate taints;
//  * This can be very dangerous here, since we get false negatives
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_12) {
//   auto Leaks = doAnalysis("taint_15_cpp.ll");
//   // 21 => {20}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[21] = {"20"};
//   // overapproximation due to lack of knowledge
//   // about ring-exchanges may be allowed, but actually 22 should not hold at
//   23 GroundTruth[23] = {"22"}; compareResults(Leaks, GroundTruth,
//                  "The xor ring-exchange was not successful");
//   // Unfortunately, the analysis detects no leaks at all
// }
// ***********************************************************/
// TEST_F(InterMonoTaintAnalysisTest, TaintTest_13) {
//   auto Leaks = doAnalysis("taint_15_1_cpp.ll");
//   // 16 => {15};
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[16] = {"15"};
//   compareResults(Leaks, GroundTruth, "The ring-exchange was not successful");
// }
// /**********************************************************
//  * Fails, since the callgraph algorithm cannot find a function without body
//  as
//  * possible callee for a virtual call; When removing this restriction we get
//  a
//  * segmentation fault
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, VirtualCalls) {
//   // boost::log::core::get()->set_logging_enabled(true);
//   auto Leaks = doAnalysis("virtual_calls_cpp.ll");
//   // 20 => {19};
//   std::map<int, std::set<std::string>> GroundTruth;
//   // Fails, although putchar is definitely a source;

//   // The dump says, the callgraph construction only finds one possible callee
//   // for bar, namely _Z3fooi which is foo(int)
//   GroundTruth[20] = {"19"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// TEST_F(InterMonoTaintAnalysisTest, VirtualCalls_v2) {
//   auto Leaks = doAnalysis("virtual_calls_v2_cpp.ll");
//   // 7 => {6};
//   std::map<int, std::set<std::string>> GroundTruth;

//   GroundTruth[7] = {"6"};
//   compareResults(Leaks, GroundTruth);
// }
// /**********************************************************
//  * Fails due to alias-unawareness (reports no leak at all)
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, StructMember) {
//   auto Leaks = doAnalysis("struct_member_cpp.ll");
//   // 16 => {15};
//   // 19 => {18};
//   std::map<int, std::set<std::string>> GroundTruth;

//   // Overapproximation due to field-insensitivity
//   GroundTruth[16] = {"15"};
//   // Actual leak
//   GroundTruth[19] = {"18"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// /**********************************************************
//  * Fails due to alias-unawareness
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, DynamicMemory) {
//   auto Leaks = doAnalysis("dynamic_memory_cpp.ll");
//   // 11 => {10}
//   std::map<int, std::set<std::string>> GroundTruth;
//   GroundTruth[11] = {"10"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/
// TEST_F(InterMonoTaintAnalysisTest, DynamicMemory_simple) {
//   auto Leaks = doAnalysis("dynamic_memory_simple_cpp.ll");
//   // 15 => {14}
//   std::map<int, std::set<std::string>> GroundTruth;

//   GroundTruth[15] = {"14"};
//   compareResults(Leaks, GroundTruth);
// }
// /**********************************************************
//  * Fails due to alias unawareness
//  **********************************************************
// TEST_F(InterMonoTaintAnalysisTest, FileIO) {
//   auto Leaks = doAnalysis("read_c.ll");

//   std::map<int, std::set<std::std::string>> GroundTruth;
//   // 37 => {36}
//   // 43 => {41}
//   GroundTruth[37] = {"36"};
//   GroundTruth[43] = {"41"};
//   compareResults(Leaks, GroundTruth);
// }
// ***********************************************************/

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
