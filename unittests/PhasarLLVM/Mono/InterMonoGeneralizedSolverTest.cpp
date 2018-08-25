#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ValueBasedContext.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

TEST(InterMonoGeneralizedSolverTest, Running) {
  ProjectIRDB IRDB(
      {PhasarDirectory +
       "build/test/llvm_test_code/control_flow/function_call_2.ll"},
      IRDBOptions::WPA);
  llvm::Module *M = IRDB.getModule(
      PhasarDirectory +
      "build/test/llvm_test_code/control_flow/function_call_2.ll");

  if (M->getFunction("main")) {
    IRDB.preprocessIR();
    LLVMTypeHierarchy H(IRDB);
    LLVMBasedICFG I(H, IRDB, CallGraphAnalysisType::OTF, {"main"});
    cout << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    InterMonoSolverTest T(I, {"main"});

    CallString<typename InterMonoSolverTest::Node_t,
               typename InterMonoSolverTest::Domain_t, 2>
        CS;
    auto S1 = make_LLVMBasedIMS(T, CS, I.getMethod("main"));
    S1->solve();

    CallString<const llvm::Value *, const llvm::Value *, 2> CS_os(
        {I.getMethod("main"), I.getMethod("function")});
    cout << CS_os << endl;

    ValueBasedContext<typename InterMonoSolverTest::Node_t,
                      typename InterMonoSolverTest::Domain_t>
        VBC;
    auto S2 = make_LLVMBasedIMS(T, VBC, I.getMethod("main"));
    S2->solve();
  } else {
    cout << "Module does not contain a 'main' function, abort!\n";
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}
