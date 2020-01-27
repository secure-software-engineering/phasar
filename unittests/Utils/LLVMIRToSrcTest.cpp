#include <gtest/gtest.h>
#include <iostream>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */

class LLVMIRToSrcTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/llvmIRtoSrc/";

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMPointsToInfo *PT;
  LLVMBasedICFG *ICFG;

  LLVMIRToSrcTest() {}
  virtual ~LLVMIRToSrcTest() {}

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG =
        new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, {"main"}, TH, PT);
  }

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete PT;
    delete ICFG;
  }
}; // Test Fixture

// TEST_F(LLVMIRToSrcTest, HandleInstructions) {
//   Initialize({pathToLLFiles + "function_call_cpp_dbg.ll"});
//   auto Fmain = ICFG->getMethod("main");
//   for (auto &BB : *Fmain) {
//     for (auto &I : BB) {
//       if (llvm::isa<llvm::StoreInst>(&I) ||
//           (llvm::isa<llvm::CallInst>(&I) &&
//            !llvm::isa<llvm::DbgValueInst>(&I) &&
//            !llvm::isa<llvm::DbgDeclareInst>(&I)) ||
//           llvm::isa<llvm::LoadInst>(&I)) {
//         std::cout << '\n'
//                   << llvmIRToString(&I) << "\n  --> "
//                   << llvmInstructionToSrc(&I) << std::endl;
//       }
//     }
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleFunctions) {
//   Initialize({pathToLLFiles + "multi_calls_cpp_dbg.ll"});
//   for (auto F : IRDB->getAllFunctions()) {
//     // F->print(llvm::outs());
//     // llvm::outs() << '\n';
//     std::cout << '\n' << llvmFunctionToSrc(F) << std::endl;
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleGlobalVariable) {
//   Initialize({pathToLLFiles + "global_01_cpp_dbg.ll"});
//   for (auto &GV :
//        IRDB->getModule(pathToLLFiles + "global_01_cpp_dbg.ll")->globals()) {
//     std::cout << '\n' << llvmGlobalValueToSrc(&GV) << std::endl;
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleAlloca) {
//   Initialize({pathToLLFiles + "function_call_cpp_dbg.ll"});
//   for (auto A : IRDB->getAllocaInstructions()) {
//     std::cout << '\n'
//               << llvmIRToString(A) << "\n  --> " << llvmValueToSrc(A)
//               << std::endl;
//   }
// }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
