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
#include <phasar/PhasarLLVM/Mono/Solver/InterMonotoneGeneralizedSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <gtest/gtest.h>

namespace bfs = boost::filesystem;

using namespace std;
using namespace psr;

TEST(InterMonotoneGeneralizedSolverTest, Running) {
  ProjectIRDB IRDB({"../../../../test/build_systems_tests/advanced_project/main.ll", "../../../../test/build_systems_tests/advanced_project/src1.ll", "../../../../test/build_systems_tests/advanced_project/src2.ll"}, IRDBOptions::WPA);
  llvm::Module *M = IRDB.getModule("../../../../test/build_systems_tests/advanced_project/main.ll");

  if (M->getFunction("main")) {
    IRDB.preprocessIR();
    LLVMTypeHierarchy H(IRDB);
    LLVMBasedICFG I(H, IRDB, WalkerStrategy::Pointer, ResolveStrategy::OTF,
                    {"main"});
    llvm::outs() << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    InterMonotoneSolverTest T(I, {"main"});

    CallString<const llvm::Value *, const llvm::Value*, 2> CS;
    InterMonotoneGeneralizedSolver<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::Value *, LLVMBasedICFG&,
                                  CallString<const llvm::Value *, const llvm::Value*, 2>> S1(T, CS, I.getMethod("main"));
    S1.solve();

    CallString<const llvm::Value *, const llvm::Value*, 2> CS_os({I.getMethod("main"), I.getMethod("function")});
    std::cout << CS_os << std::endl;

    ValueBasedContext<const llvm::Value *, const llvm::Value*> VBC;
    InterMonotoneGeneralizedSolver<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::Value *, LLVMBasedICFG&,
                                  ValueBasedContext<const llvm::Value *, const llvm::Value*>> S2(T, VBC, I.getMethod("main"));

    S2.solve();
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
