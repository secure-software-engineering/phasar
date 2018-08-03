#include <iostream>
#include <memory>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <boost/filesystem/operations.hpp>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonotoneSolverTest.h>
#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ValueBasedContext.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonotoneSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <gtest/gtest.h>

namespace bfs = boost::filesystem;

using namespace std;
using namespace psr;

TEST(InterMonotoneGeneralizedSolverTest, Running) {
  ProjectIRDB IRDB({"../../../test/llvm_test_code/control_flow/function_call_2.ll"}, IRDBOptions::WPA);
  llvm::Module *M = IRDB.getModule("../../../test/llvm_test_code/control_flow/function_call_2.ll");

  if (M->getFunction("main")) {
    IRDB.preprocessIR();
    LLVMTypeHierarchy H(IRDB);
    LLVMBasedICFG I(H, IRDB, CallGraphAnalysisType::OTF,
                    {"main"});
    llvm::outs() << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    InterMonotoneSolverTest T(I, {"main"});

    CallString<typename InterMonotoneSolverTest::Node_t, typename InterMonotoneSolverTest::Domain_t, 2> CS;
    auto S1 = make_LLVMBasedIMS(T, CS, I.getMethod("main"));
    S1->solve();

    CallString<const llvm::Value *, const llvm::Value*, 2> CS_os({I.getMethod("main"), I.getMethod("function")});
    std::cout << CS_os << std::endl;


    ValueBasedContext<typename InterMonotoneSolverTest::Node_t, typename InterMonotoneSolverTest::Domain_t> VBC;
    auto S2 = make_LLVMBasedIMS(T, VBC, I.getMethod("main"));
    S2->solve();
  } else {
    llvm::outs() << "Module does not contain a 'main' function, abort!\n";
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
  llvm::llvm_shutdown();

  return result;
}
