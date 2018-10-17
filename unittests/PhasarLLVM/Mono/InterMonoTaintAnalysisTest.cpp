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

TEST(InterMonoTaintAnalysisTest, Running) {
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
    InterMonoTaintAnalysis IMTaintAnalysis(I, {"main"});

    CallString<typename InterMonoTaintAnalysis::Node_t,
               typename InterMonoTaintAnalysis::Domain_t, 2>
        CS(&IMTaintAnalysis, &IMTaintAnalysis);
    cout << "Print call string\n" << CS << endl;
    auto S1 = make_LLVMBasedIMS(IMTaintAnalysis, CS, I.getMethod("main"));
    S1->solve();

    struct IMSTestDPrinter : DataFlowFactPrinter<string> {
      void printDataFlowFact(std::ostream &os, string d) const override {
        os << d;
      }
    };
    struct IMSTestNPrinter : NodePrinter<string> {
      void printNode(std::ostream &os, string n) const { os << n; }
    };
    IMSTestDPrinter IMSTestDP;
    IMSTestNPrinter IMSTestNP;
    CallString<string, string, 2> CS_os(&IMSTestNP, &IMSTestDP, {"foo", "bar"});
    cout << CS_os << endl;

    ValueBasedContext<typename InterMonoTaintAnalysis::Node_t,
                      typename InterMonoTaintAnalysis::Domain_t>
        VBC(&IMTaintAnalysis, &IMTaintAnalysis);
    auto S2 = make_LLVMBasedIMS(IMTaintAnalysis, VBC, I.getMethod("main"));
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
