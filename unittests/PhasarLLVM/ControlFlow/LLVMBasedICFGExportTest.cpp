#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

using nlohmann::json;

class LLVMBasedICFGExportTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      unittest::PathToLLTestFiles + "call_graphs/";

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  json exportICFG(const std::string &testFile) {
    ProjectIRDB IRDB({pathToLLFiles + testFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);

    return ICFG.exportICFGAsJson();
  }
};

TEST_F(LLVMBasedICFGExportTest, ExportICFG01) {
  auto results = exportICFG("static_callsite_8_cpp.ll");
  std::cerr << results.dump(4) << std::endl;
  // TODO: Add ground truth
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}