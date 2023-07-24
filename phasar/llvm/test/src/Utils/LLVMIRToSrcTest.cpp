#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */

class LLVMIRToSrcTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles = PHASAR_BUILD_SUBFOLDER("llvmIRtoSrc/");

  unique_ptr<LLVMProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMAliasSet> PT;
  unique_ptr<LLVMBasedICFG> ICFG;

  LLVMIRToSrcTest() = default;
  ~LLVMIRToSrcTest() override = default;

  void initialize(const llvm::Twine &IRFile) {
    IRDB = make_unique<LLVMProjectIRDB>(IRFile);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMAliasSet>(IRDB.get());
    auto EntryPoints = {"main"s};
    ICFG = make_unique<LLVMBasedICFG>(IRDB.get(), CallGraphAnalysisType::OTF,
                                      EntryPoints, TH.get(), PT.get());
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}
}; // Test Fixture

// TEST_F(LLVMIRToSrcTest, HandleInstructions) {
//   Initialize({pathToLLFiles + "function_call.dbg.ll"});
//   auto Fmain = ICFG->getMethod("main");
//   for (auto &BB : *Fmain) {
//     for (auto &I : BB) {
//       if (llvm::isa<llvm::StoreInst>(&I) ||
//           (llvm::isa<llvm::CallInst>(&I) &&
//            !llvm::isa<llvm::DbgValueInst>(&I) &&
//            !llvm::isa<llvm::DbgDeclareInst>(&I)) ||
//           llvm::isa<llvm::LoadInst>(&I)) {
//         llvm::outs() << '\n'
//                   << llvmIRToString(&I) << "\n  --> "
//                   << llvmInstructionToSrc(&I) << std::endl;
//       }
//     }
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleFunctions) {
//   Initialize({pathToLLFiles + "multi_calls.dbg.ll"});
//   for (auto F : IRDB->getAllFunctions()) {
//     // F->print(llvm::outs());
//     // llvm::outs() << '\n';
//     llvm::outs() << '\n' << llvmFunctionToSrc(F) << std::endl;
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleGlobalVariable) {
//   Initialize({pathToLLFiles + "global_01.dbg.ll"});
//   for (auto &GV :
//        IRDB->getModule(pathToLLFiles + "global_01.dbg.ll")->globals()) {
//     llvm::outs() << '\n' << llvmGlobalValueToSrc(&GV) << std::endl;
//   }
// }

// TEST_F(LLVMIRToSrcTest, HandleAlloca) {
//   Initialize({pathToLLFiles + "function_call.dbg.ll"});
//   for (auto A : IRDB->getAllocaInstructions()) {
//     llvm::outs() << '\n'
//               << llvmIRToString(A) << "\n  --> " << llvmValueToSrc(A)
//               << std::endl;
//   }
// }
