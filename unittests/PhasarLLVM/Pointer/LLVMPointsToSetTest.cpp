#include "gtest/gtest.h"

#include <string>

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

using namespace psr;

class LLVMBasedICFGTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFGTest, Intra_01) {
  ProjectIRDB IRDB({PathToLlFiles + "pointers/basic_01_cpp_dbg.ll"});
  LLVMPointsToSet PTS(IRDB, false);
  auto *Main = IRDB.getFunctionDefinition("main");
  for (auto &BB : *Main) {
    for (auto &I : BB) {
      auto S = PTS.getPointsToSet(&I);
    }
  }
  PTS.print(std::cout);
  std::cout << '\n';
}

TEST_F(LLVMBasedICFGTest, Inter_01) {
  ProjectIRDB IRDB({PathToLlFiles + "pointers/call_01_cpp_dbg.ll"});
  LLVMPointsToSet PTS(IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PTS);
  auto *Main = IRDB.getFunctionDefinition("main");
  for (auto &BB : *Main) {
    for (auto &I : BB) {
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