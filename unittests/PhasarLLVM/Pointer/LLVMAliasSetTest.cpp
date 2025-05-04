#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

TEST(LLVMAliasSet, Intra_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/basic_01_cpp.ll");

  LLVMAliasSet PTS(&IRDB, false);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      std::ignore = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}

TEST(LLVMAliasSet, Inter_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles + "pointers/call_01_cpp.ll");
  LLVMAliasSet PTS(&IRDB, false);
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      std::ignore = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}

TEST(LLVMAliasSet, Global_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "pointers/global_01_cpp.ll");
  LLVMAliasSet PTS(&IRDB, false);
  DIBasedTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &G : Main->getParent()->globals()) {
    std::ignore = PTS.getAliasSet(&G); // NOLINT
  }
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      std::ignore = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
