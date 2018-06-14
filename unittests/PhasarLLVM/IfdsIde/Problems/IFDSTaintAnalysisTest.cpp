#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace psr;

class IFDSTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(IFDSTaintAnalysisTest, HandleControlFlow) {
  initializeLogger(true);
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call.ll"},
                   IRDBOptions::NONE);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF,
                     {"main"});
  IFDSTaintAnalysis TaintProblem(ICFG, {"main"});
  LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> TaintSolver(TaintProblem,
                                                                   true);
  TaintSolver.solve();
  std::cout << "Problem has been solved" << std::endl;

  // TaintSolver.ifdsResultsAt()
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
