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
      PhasarConfig::getPhasarConfig().PhasarDirectory() + "build/test/llvm_test_code/taint_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

  // @ retrun the number of tained Instruction
  int computeCounterResult(
      MonoMap<const llvm::Instruction *,
              MonoMap<CallStringCTX<const llvm::Value *,
                                    const llvm::Instruction *, 3>,
                      MonoSet<const llvm::Value *>>> &Analysis, ProjectIRDB &IRDB, unsigned InstNum) {
    llvm::Function *F = IRDB.getFunction("main");
    const llvm::Instruction * Inst = getNthInstruction(F, InstNum);
    int counter = 0;
    // count the number of facts after investigating the last Instruction
    for (auto &entry : Analysis) {
      //if (!entry.second.empty() && llvm::isa<llvm::ReturnInst>(entry.first)) {
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
    const llvm::Instruction * Inst = getNthInstruction(F, InstNum);
    for (auto &entry : Analysis) {
      int SinkId = stoi(getMetaDataID(entry.first));
      cout<<"SinkId: "<< SinkId<<endl;
      set<string> LeakedValueIds;
      //if (llvm::isa<llvm::ReturnInst>(entry.first)){
      if (Inst == entry.first){
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
      TaintProblem, true);
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
      TaintProblem, true);
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
      TaintProblem, true);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 9);

  Facts = set<string>{"60", "61", "62", "64", "67", "68", "72", "main.0", "main.1"};
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
      TaintProblem, true);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 7);

  Facts= set<string>{"100", "101", "102", "107", "108", "main.0", "main.1"};
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
      TaintProblem, true);
  TaintSolver.solve();
  MonoMap<
      const llvm::Instruction *,
      MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>,
              MonoSet<const llvm::Value *>>>
      Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(Analysis, IRDB, InstNum);
  ASSERT_EQ(counter, 8);

  Facts = set<string>{"125", "126", "127", "133", "134", "138", "main.0", "main.1"};
  compareResults(Analysis, Facts, IRDB, InstNum);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}