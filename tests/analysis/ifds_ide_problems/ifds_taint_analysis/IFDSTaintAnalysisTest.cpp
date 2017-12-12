#include <gtest/gtest.h>
#include <vector>
#include "../../../../src/analysis/control_flow/LLVMBasedICFG.h"
#include "../../../../src/analysis/ifds_ide/solver/LLVMIFDSSolver.h"
#include "../../../../src/analysis/ifds_ide_problems/ifds_taint_analysis/IFDSTaintAnalysis.h"
#include "../../../../src/analysis/points-to/LLVMTypeHierarchy.h"
#include "../../../../src/db/ProjectIRDB.h"
using namespace std;

TEST(SecondTest1, SecondTestName1) {
  ProjectIRDB IRDB({"test_code/llvm_test_code/control_flow/function_call.ll"}, IRDBOptions::NONE);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF, {"main"});
  IFDSTaintAnalysis TaintProblem(ICFG, {"main"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(TaintProblem,
                                                                   true);
  TaintSolver.solve();
	cout << "Problem has been solved" << endl;
  
	// TaintSolver.ifdsResultsAt()
}

TEST(SecondTest2, SecondTestName2) {
  vector<int> iv = {1, 2, 3};
  ASSERT_EQ(3, iv.size());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
