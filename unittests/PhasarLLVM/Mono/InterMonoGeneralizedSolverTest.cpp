#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ValueBasedContext.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonoSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>

using namespace std;
using namespace psr;

TEST(InterMonoGeneralizedSolverTest, Running) {
  ProjectIRDB IRDB(
      {PhasarDirectory +
       "build/test/llvm_test_code/control_flow/function_call_2_cpp.ll"},
      IRDBOptions::WPA);
  llvm::Module *M = IRDB.getModule(
      PhasarDirectory +
      "build/test/llvm_test_code/control_flow/function_call_2_cpp.ll");

  if (M->getFunction("main")) {
    IRDB.preprocessIR();
    LLVMTypeHierarchy H(IRDB);
    LLVMBasedICFG I(H, IRDB, CallGraphAnalysisType::OTF, {"main"});
    cout << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    InterMonoSolverTest IMSTest(I, {"main"});

    struct IMSTestDPrinter
        : DataFlowFactPrinter<InterMonoSolverTest::Domain_t> {
      void printDataFlowFact(std::ostream &os,
                             InterMonoSolverTest::Domain_t d) const override {
        for (auto fact : d) {
          os << llvmIRToString(fact) << '\n';
        }
      }
    };
    struct IMSTestNPrinter : NodePrinter<InterMonoSolverTest::Node_t> {
      void printNode(std::ostream &os, InterMonoSolverTest::Node_t n) const {
        os << llvmIRToString(n);
      }
    };
    auto IMSTestDP = new unique_ptr<IMSTestDPrinter>();
    auto IMSTestNP = new unique_ptr<IMSTestNPrinter>();
    CallString<typename InterMonoSolverTest::Node_t,
               typename InterMonoSolverTest::Domain_t, 2>
        CS(IMSTestNP->get(), IMSTestDP->get());
    auto S1 = make_LLVMBasedIMS(IMSTest, CS, I.getMethod("main"));
    S1->solve();

    ValueBasedContext<typename InterMonoSolverTest::Node_t,
                      typename InterMonoSolverTest::Domain_t>
        VBC(IMSTestNP->get(), IMSTestDP->get());
    auto S2 = make_LLVMBasedIMS(IMSTest, VBC, I.getMethod("main"));
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
