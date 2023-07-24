#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

TEST(LLVMAliasSet, Intra_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB({"llvm_test_code/pointers/basic_01.ll"});

  LLVMAliasSet PTS(&IRDB, false);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}

TEST(LLVMAliasSet, Inter_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB({"llvm_test_code/pointers/call_01.ll"});
  LLVMAliasSet PTS(&IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}

TEST(LLVMAliasSet, Global_01) {
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB({"llvm_test_code/pointers/global_01.ll"});
  LLVMAliasSet PTS(&IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &G : Main->getParent()->globals()) {
    auto S = PTS.getAliasSet(&G); // NOLINT
  }
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getAliasSet(&I); // NOLINT
    }
  }
  PTS.print(llvm::outs());
  llvm::outs() << '\n';
}
