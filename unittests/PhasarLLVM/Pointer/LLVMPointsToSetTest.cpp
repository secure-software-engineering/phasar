#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"

using namespace psr;

TEST(LLVMPointsToSet, Intra_01) {
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

TEST(LLVMPointsToSet, SaveAndLoad) {
  std::vector<std::string> testcases = {"pointers/basic_01_cpp.ll", "pointers/global_01_cpp.ll"};

  for (auto &tc : testcases) {
    ProjectIRDB IRDB1({unittest::PathToLLTestFiles + tc});
    LLVMPointsToSet PTS1(IRDB1, false);
    PTS1.save("./points_to_set", IRDB1);

    ProjectIRDB IRDB2({unittest::PathToLLTestFiles + tc});
    LLVMPointsToSet PTS2(IRDB2, std::string("./points_to_set"));

    std::cout << "[PTS1]" << std::endl;
    PTS1.print(std::cout);

    std::cout << "[PTS2]" << std::endl;
    PTS2.print(std::cout);
  }
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
