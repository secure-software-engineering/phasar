#include <gtest/gtest.h>
#include <llvm/Support/raw_ostream.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/CallString.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

/**
 * The MetaDataIDs depend on execution-order.
 * Only check the tainted values, not the actual leaks.
 * Only check taints in main. There might be functions called by main, which
 * both contain sources and leaks, such that there ads no DataFlowFacts holding
 * in main at all.
 */

using namespace std;
using namespace psr;

class InterMonoTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/taint_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

#pragma region Environment for leak checking
  ProjectIRDB *IRDB = nullptr;
  void SetUp() override { bl::core::get()->set_logging_enabled(false); }
  void TearDown() override {
    if (IRDB) {
      delete IRDB;
      IRDB = nullptr;
    }
  }
  const map<llvm::Instruction const *, set<llvm::Value const *>>
  doAnalysis(std::string llvmFilePath, bool printDump = false) {
    // make IRDB dynamic, such that the llvm::Value* and llvm::Instruction* live
    // longer than this method (they are needed in compareResults)
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath});
    ValueAnnotationPass::resetValueID();
    IRDB->preprocessIR();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMBasedICFG ICFG(TH, *IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
    LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
        TaintProblem);
    TaintSolver.solve();
    if (printDump) {
      TaintSolver.dumpResults();
    }
    return TaintProblem.getAllLeaks();
  }
  void compareResults(
      map<llvm::Instruction const *, set<llvm::Value const *>> &Leaks,
      map<int, set<string>> &GroundTruth, string errorMessage = "") {
    map<int, set<string>> LeakIds;
    for (const auto &kvp : Leaks) {
      int InstId = stoi(getMetaDataID(kvp.first));
      EXPECT_NE(-1, InstId);
      for (auto leakVal : kvp.second) {
        LeakIds[InstId].insert(getMetaDataID(leakVal));
      }
    }
    EXPECT_EQ(LeakIds, GroundTruth) << errorMessage;
  }
#pragma endregion

  int computeCounterResult(
      MonoMap<const llvm::Instruction *,
              MonoMap<CallStringCTX<const llvm::Value *,
                                    const llvm::Instruction *, 3>,
                      MonoSet<const llvm::Value *>>> &Analysis,
      ProjectIRDB &IRDB, unsigned InstNum) {
    llvm::Function *F = IRDB.getFunction("main");
    const llvm::Instruction *Inst = getNthInstruction(F, InstNum);
    int counter = 0;
    // count the number of facts after investigating the last Instruction
    for (auto &entry : Analysis) {
      // if (!entry.second.empty() && llvm::isa<llvm::ReturnInst>(entry.first))
      // {
      if (!entry.second.empty() && Inst == entry.first) {
        for (auto &context : entry.second) {
          if (!context.second.empty()) {
            for (auto &fact : context.second) {
              counter++;
            }
          }
        }
      }
    }
    return counter;
  }

  void
  compareResults(MonoMap<const llvm::Instruction *,
                         MonoMap<CallStringCTX<const llvm::Value *,
                                               const llvm::Instruction *, 3>,
                                 MonoSet<const llvm::Value *>>> &Analysis,
                 set<string> &Facts, ProjectIRDB &IRDB, unsigned InstNum) {
    llvm::Function *F = IRDB.getFunction("main");
    set<string> FoundLeaks;
    const llvm::Instruction *Inst = getNthInstruction(F, InstNum);
    for (auto &entry : Analysis) {
      int SinkId = stoi(getMetaDataID(entry.first));
      cout << "SinkId: " << SinkId << endl;
      set<string> LeakedValueIds;
      // if (llvm::isa<llvm::ReturnInst>(entry.first)){
      if (Inst == entry.first) {
        for (auto &context : entry.second) {
          if (!context.second.empty()) {
            for (auto &fact : context.second) {
              LeakedValueIds.insert(getMetaDataID(fact));
            }
          }
          FoundLeaks = LeakedValueIds;
        }
      }
    }
    EXPECT_EQ(FoundLeaks, Facts);
  }
};

TEST_F(InterMonoTaintAnalysisTest, TaintTest_01) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_9_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  set<string> Facts;
  unsigned InstNum = 9;

  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();

  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 7);

  Facts = set<string>{"10", "11", "5", "6", "7", "main.0", "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_02) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_10_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  set<string> Facts;
  unsigned InstNum = 15;

  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 7);

  Facts = set<string>{"21", "22", "23", "28", "29", "main.0", "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_03) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_11_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  set<string> Facts;
  unsigned InstNum = 15;

  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 9);

  Facts =
      set<string>{"60", "61", "62", "64", "67", "68", "72", "main.0", "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_04) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_12_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  set<string> Facts;
  unsigned InstNum = 15;

  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 7);

  Facts = set<string>{"100", "101", "102", "107", "108", "main.0", "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_05) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_13_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  set<string> Facts;
  unsigned InstNum = 25;

  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
      TaintProblem);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  EXPECT_EQ(counter, 7);
  // 125 does no longer hold due to killing facts on store
  Facts =
      set<string>{/*"125", */ "126", "127",   "133", "134", "138",
                  "main.0",          "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}
/******************************************************************************
 * Tests actually based on leaked values, not on dataflow facts
 *
 ******************************************************************************/

TEST_F(InterMonoTaintAnalysisTest, TaintTest_01_v2) {
  auto Leaks = doAnalysis("taint_9_c.ll");
  // 14 => {13}
  map<int, set<string>> GroundTruth;
  GroundTruth[14] = {"13"};
  compareResults(Leaks, GroundTruth);
}
TEST_F(InterMonoTaintAnalysisTest, TaintTest_02_v2) {
  auto Leaks = doAnalysis("taint_10_c.ll");
  // 20 => {19}
  map<int, set<string>> GroundTruth;
  GroundTruth[20] = {"19"};
  compareResults(Leaks, GroundTruth);
}
TEST_F(InterMonoTaintAnalysisTest, TaintTest_03_v2) {
  // bl::core::get()->set_logging_enabled(true);
  auto Leaks = doAnalysis("taint_11_c.ll");
  // 35 => {34}
  // 37 => {36} due to overapproximation (limitation of call string)
  map<int, set<string>> GroundTruth;
  GroundTruth[35] = {"34"};
  GroundTruth[37] = {"36"};
  // kind of nondeterministic: sometimes it only leaks at 35, some only at
  // 37, but most times at both
  compareResults(Leaks, GroundTruth);
}
TEST_F(InterMonoTaintAnalysisTest, TaintTest_04_v2) {
  auto Leaks = doAnalysis("taint_12_c.ll");
  // 36 => {35}
  // why not 38 => {37} due to overapproximation in recursion (limitation of
  // call string) ???
  map<int, set<string>> GroundTruth;
  GroundTruth[36] = {"35"};
  // GroundTruth[38] = {"37"};
  compareResults(Leaks, GroundTruth);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_05_v2) {
  auto Leaks = doAnalysis("taint_13_c.ll");
  // 32 => {31}
  // 34 => {33} will not leak (analysis is naturally never strong enough for
  // this)
  map<int, set<string>> GroundTruth;
  GroundTruth[32] = {"31"};
  compareResults(Leaks, GroundTruth);
}
/**********************************************************
 * fails due to alias-unawareness
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, TaintTest_06) {
  auto Leaks = doAnalysis("taint_4_v2_cpp.ll");
  // 19 => {18}
  map<int, set<string>> GroundTruth;
  GroundTruth[19] = {"18"};

  compareResults(Leaks, GroundTruth);
}
***********************************************************/
/**********************************************************
 * fails, since std::cout is not a sink
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, TaintTest_07) {
  auto Leaks = doAnalysis("taint_2_v2_cpp.ll");
  // 10 => {9}
  map<int, set<string>> GroundTruth;
  GroundTruth[10] = {"9"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
TEST_F(InterMonoTaintAnalysisTest, TaintTest_08) {
  auto Leaks = doAnalysis("taint_2_v2_1_cpp.ll");
  // 4 => {3}
  map<int, set<string>> GroundTruth;
  GroundTruth[4] = {"3"};
  compareResults(Leaks, GroundTruth);
}
/**********************************************************
 * fails due to lack of alias information
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, TaintTest_09) {
  auto Leaks = doAnalysis("source_sink_function_test_c.ll");
  // 41 => {40};
  map<int, set<string>> GroundTruth;
  GroundTruth[41] = {"40"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
TEST_F(InterMonoTaintAnalysisTest, TaintTest_10) {
  auto Leaks = doAnalysis("taint_14_cpp.ll");
  // 11 => {10}; do not know, why it fails; getchar is definitely a source, but
  // it doesn't generate a fact
  map<int, set<string>> GroundTruth;
  GroundTruth[11] = {"10"};
  compareResults(Leaks, GroundTruth);
}
/**********************************************************
 * fails, because arithmetic operations do not propagate taints;
 * In contrast to TaintTest10, getchar generates a fact here;
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, TaintTest_11) {
  auto Leaks = doAnalysis("taint_14_1_cpp.ll");
  // 12 => {11}; quite similar as TaintTest10, but all in main;
  map<int, set<string>> GroundTruth;
  GroundTruth[12] = {"11"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
/**********************************************************
 * fails, since arithmetic operations do not propagate taints;
 * This can be very dangerous here, since we get false negatives
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, TaintTest_12) {
  auto Leaks = doAnalysis("taint_15_cpp.ll");
  // 21 => {20}
  map<int, set<string>> GroundTruth;
  GroundTruth[21] = {"20"};
  // overapproximation due to lack of knowledge
  // about ring-exchanges may be allowed, but actually 22 should not hold at 23
  GroundTruth[23] = {"22"};
  compareResults(Leaks, GroundTruth,
                 "The xor ring-exchange was not successful");
  // Unfortunately, the analysis detects no leaks at all
}
***********************************************************/
TEST_F(InterMonoTaintAnalysisTest, TaintTest_13) {
  auto Leaks = doAnalysis("taint_15_1_cpp.ll");
  // 16 => {15};
  map<int, set<string>> GroundTruth;
  GroundTruth[16] = {"15"};
  compareResults(Leaks, GroundTruth, "The ring-exchange was not successful");
}
/**********************************************************
 * Fails, since the callgraph algorithm cannot find a function without body as
 * possible callee for a virtual call; When removing this restriction we get a
 * segmentation fault
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, VirtualCalls) {
  // bl::core::get()->set_logging_enabled(true);
  auto Leaks = doAnalysis("virtual_calls_cpp.ll");
  // 20 => {19};
  map<int, set<string>> GroundTruth;
  // Fails, although putchar is definitely a source;

  // The dump says, the callgraph construction only finds one possible callee
  // for bar, namely _Z3fooi which is foo(int)
  GroundTruth[20] = {"19"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
TEST_F(InterMonoTaintAnalysisTest, VirtualCalls_v2) {
  auto Leaks = doAnalysis("virtual_calls_v2_cpp.ll");
  // 7 => {6};
  map<int, set<string>> GroundTruth;

  GroundTruth[7] = {"6"};
  compareResults(Leaks, GroundTruth);
}
/**********************************************************
 * Fails due to alias-unawareness (reports no leak at all)
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, StructMember) {
  auto Leaks = doAnalysis("struct_member_cpp.ll");
  // 16 => {15};
  // 19 => {18};
  map<int, set<string>> GroundTruth;

  // Overapproximation due to field-insensitivity
  GroundTruth[16] = {"15"};
  // Actual leak
  GroundTruth[19] = {"18"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
/**********************************************************
 * Fails due to alias-unawareness
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, DynamicMemory) {
  auto Leaks = doAnalysis("dynamic_memory_cpp.ll");
  // 11 => {10}
  map<int, set<string>> GroundTruth;
  GroundTruth[11] = {"10"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/
TEST_F(InterMonoTaintAnalysisTest, DynamicMemory_simple) {
  auto Leaks = doAnalysis("dynamic_memory_simple_cpp.ll");
  // 15 => {14}
  map<int, set<string>> GroundTruth;

  GroundTruth[15] = {"14"};
  compareResults(Leaks, GroundTruth);
}
/**********************************************************
 * Fails due to alias unawareness
 **********************************************************
TEST_F(InterMonoTaintAnalysisTest, FileIO) {
  auto Leaks = doAnalysis("read_c.ll");

  map<int, set<string>> GroundTruth;
  // 37 => {36}
  // 43 => {41}
  GroundTruth[37] = {"36"};
  GroundTruth[43] = {"41"};
  compareResults(Leaks, GroundTruth);
}
***********************************************************/

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}