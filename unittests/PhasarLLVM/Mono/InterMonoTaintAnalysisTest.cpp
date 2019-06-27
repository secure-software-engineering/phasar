#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/CallString.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;


class InterMonoTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/taint_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

// @ retrun the number of tained Instruction
int computeCounterResult(MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> &Analysis){
  int counter = 0;
  // count the number of facts after investigating the last Instruction
  for (auto &entry :Analysis) {
      if (!entry.second.empty()) {
        for (auto &context : entry.second) {
          counter = 0;
          if (!context.second.empty()) {
            for (auto &fact : context.second) {
              counter ++;
            }
          }
        }
      }
    }
    return counter;
  }

void compareResults(MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> &Analysis, map<int, set<string>> &Facts ){
   map<int, set<string>> FoundLeaks;
   map<int, set<string>> TempLeaks;
    for (auto &entry : Analysis) {
      int SinkId = stoi(getMetaDataID(entry.first));
      set<string> LeakedValueIds;
      FoundLeaks = TempLeaks;
      for (auto &context : entry.second) {
          if (!context.second.empty()) {
            for (auto &fact : context.second) {
              LeakedValueIds.insert(getMetaDataID(fact));
            }
          }
        }
        FoundLeaks.insert(make_pair(SinkId, LeakedValueIds));
    }
    EXPECT_EQ(FoundLeaks, Facts);
  }
};


TEST_F(InterMonoTaintAnalysisTest, TaintTest_01) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_9_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);

  LLVMBasedICFG ICFG (TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
            TaintProblem, true);
  TaintSolver.solve();

   MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis);
  ASSERT_EQ(counter, 8);

  map<int, set<string>> Facts;
  Facts[15] = set<string>{ "10", "11", "13", "5", "6", "7", "main.0", "main.1" };
  compareResults(Analysis, Facts);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_02) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_10_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);

  LLVMBasedICFG ICFG (TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
            TaintProblem, true);
  TaintSolver.solve();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis);
  ASSERT_EQ(counter, 1);

  map<int, set<string>> Facts;
  Facts[17] = set<string>{"_Z3fooi.0"};
  compareResults(Analysis, Facts);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_03) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_11_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);

  LLVMBasedICFG ICFG (TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
            TaintProblem, true);
  TaintSolver.solve();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis);
  ASSERT_EQ(counter, 3);

  map<int, set<string>> Facts;
  Facts[44] = set<string>{ "41", "43", "_Z3quki.0" };
  compareResults(Analysis, Facts);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_04) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_12_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);

  LLVMBasedICFG ICFG (TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
            TaintProblem, true);
  TaintSolver.solve();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis);
  ASSERT_EQ(counter, 0);

  map<int, set<string>> Facts;
  Facts[96] = set<string>{"81", "91", "_Z3fooii.0" };
  compareResults(Analysis, Facts);
}

TEST_F(InterMonoTaintAnalysisTest, TaintTest_05) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_13_c.ll"}, IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);

  LLVMBasedICFG ICFG (TH, IRDB, CallGraphAnalysisType::OTF, EntryPoints);
  InterMonoTaintAnalysis TaintProblem(ICFG, EntryPoints);
  LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> TaintSolver(
            TaintProblem, true);
  TaintSolver.solve();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis);
  //ASSERT_EQ(counter, 0);

  map<int, set<string>> Facts;
  Facts[120] = set<string>{"_Z3fooi.0"};
  compareResults(Analysis, Facts);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}
