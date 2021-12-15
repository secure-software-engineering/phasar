#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "llvm/ADT/StringRef.h"

#include "nlohmann/json.hpp"

#include "TestConfig.h"

using namespace psr;

using SetTy = std::set<std::set<std::string>>;
using GroundTruthTy = std::pair<SetTy, std::set<std::string>>;

static std::set<std::string> makeInnerSet(const nlohmann::json &Arr) {
  std::set<std::string> Inner;
  for (const auto &Elem : Arr) {
    Inner.insert(Elem.get<std::string>());
  }
  return Inner;
}

static SetTy makeSet(const nlohmann::json &J) {
  // assume, we are given an array of arrays
  SetTy Ret;
  for (const auto &Arr : J) {
    Ret.insert(makeInnerSet(Arr));
  }
  return Ret;
}

static void analyze(llvm::StringRef File, const GroundTruthTy &Gt,
                    llvm::StringRef EntryPoint = "main") {
  ValueAnnotationPass::resetValueID();
  ProjectIRDB IRDB({unittest::PathToLLTestFiles + File.str()});

  // llvm::outs() << *IRDB.getWPAModule() << '\n';

  LLVMPointsToSet PTS(IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(IRDB, CallGraphAnalysisType::OTF, {EntryPoint.str()}, &TH,
                    &PTS);

  auto Ser = PTS.getAsJson();

  ASSERT_TRUE(Ser.count("PointsToSets"));
  ASSERT_TRUE(Ser.count("AnalyzedFunctions"));

  const auto &PSets = Ser.at("PointsToSets");
  const auto &Funs = Ser.at("AnalyzedFunctions");

  const auto &GtPSets = Gt.first;
  const auto &GtFuns = Gt.second;

  EXPECT_EQ(GtPSets.size(), PSets.size());
  EXPECT_EQ(GtFuns.size(), Funs.size());

  auto PSetsSet = makeSet(PSets);
  auto FunsSet = makeInnerSet(Funs);

  EXPECT_EQ(GtPSets, PSetsSet);
  EXPECT_EQ(GtFuns, FunsSet);
}

TEST(LLVMPointsToSetSerializationTest, Ser_Intra01) {
  analyze("pointers/basic_01_cpp.ll", {{{"1"}, {"0", "3"}}, {"main"}});
}

TEST(LLVMPointsToSetSerializationTest, Ser_Inter01) {
  analyze("pointers/call_01_cpp.ll",
          {{{"0"}, {"10", "12", "2", "6", "_Z10setIntegerPi.0"}, {"5"}, {"7"}},
           {"main", "_Z10setIntegerPi"}});
}

TEST(LLVMPointsToSetSerializationTest, Ser_Global01) {
  GTEST_SKIP() << "TODO: Add ground truth!!!";
  analyze("pointers/global_01_cpp.ll", {{}, {}});
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
