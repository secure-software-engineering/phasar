// #include <iostream>
// #include <memory>

// #include <boost/filesystem/operations.hpp>
// #include <llvm/IR/LLVMContext.h>
// #include <llvm/IR/Module.h>
// #include <llvm/IR/Verifier.h>
// #include <llvm/IRReader/IRReader.h>
// #include <llvm/Support/SourceMgr.h>

#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ValueBasedContext.h>
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

// @ retruned the number of tained Instruction
int computeCounterResult(LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3> & TaintSolver, 
                          MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> &Analysis){
  int counter = 0;
  // count the number of facts after investigating the last Instruction
  for (auto &entry : TaintSolver.getAnalysis()) {
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
  cout<<"analyis size is: "<<TaintSolver.getAnalysis().size();

   MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(TaintSolver, Analysis );
  cout<<"counter is: "<<counter;
  ASSERT_EQ(counter, 8);
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
  cout<<"analyis size is: "<<TaintSolver.getAnalysis().size();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(TaintSolver, Analysis );
  cout<<"counter is: "<<counter;
  ASSERT_EQ(counter, 1);
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
  cout<<"analyis size is: "<<TaintSolver.getAnalysis().size();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(TaintSolver, Analysis );
  cout<<"counter is: "<<counter;
  ASSERT_EQ(counter, 3);
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
  cout<<"analyis size is: "<<TaintSolver.getAnalysis().size();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(TaintSolver, Analysis );
  cout<<"counter is: "<<counter;
  ASSERT_EQ(counter, 0);
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
  cout<<"analyis size is: "<<TaintSolver.getAnalysis().size();
  MonoMap<const llvm::Instruction *, MonoMap<CallStringCTX<const llvm::Value *, const llvm::Instruction *, 3>, MonoSet<const llvm::Value *>>> Analysis = TaintSolver.getAnalysis();

  int counter = computeCounterResult(TaintSolver, Analysis );
  cout<<"counter is: "<<counter;
  ASSERT_EQ(counter, 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}
