#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"

using namespace psr;

TEST(LLVMPointsToSet, Intra_01) {
  ValueAnnotationPass::resetValueID();
  ProjectIRDB IRDB({unittest::PathToLLTestFiles + "pointers/basic_01_cpp.ll"});

  LLVMPointsToSet PTS(IRDB, false);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getPointsToSet(&I);
    }
  }
  PTS.print(std::cout);
  std::cout << '\n';
}

TEST(LLVMPointsToSet, Inter_01) {
  ValueAnnotationPass::resetValueID();
  ProjectIRDB IRDB({unittest::PathToLLTestFiles + "pointers/call_01_cpp.ll"});
  LLVMPointsToSet PTS(IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getPointsToSet(&I);
    }
  }
  PTS.print(std::cout);
  std::cout << '\n';
}

TEST(LLVMPointsToSet, Global_01) {
  ValueAnnotationPass::resetValueID();
  ProjectIRDB IRDB({unittest::PathToLLTestFiles + "pointers/global_01_cpp.ll"});
  LLVMPointsToSet PTS(IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  const auto *Main = IRDB.getFunctionDefinition("main");
  for (const auto &G : Main->getParent()->globals()) {
    auto S = PTS.getPointsToSet(&G);
  }
  for (const auto &BB : *Main) {
    for (const auto &I : BB) {
      auto S = PTS.getPointsToSet(&I);
    }
  }
  PTS.print(std::cout);
  std::cout << '\n';
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
