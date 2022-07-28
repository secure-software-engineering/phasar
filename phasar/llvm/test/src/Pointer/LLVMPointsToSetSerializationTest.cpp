#include <cstdlib>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

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

static void checkSer(const nlohmann::json &Ser, const GroundTruthTy &Gt) {

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

static void checkDeser(const llvm::Module &Mod, LLVMPointsToSet &PTS,
                       LLVMPointsToSet &Deser) {
  for (const auto &Glob : Mod.globals()) {
    EXPECT_EQ(*PTS.getPointsToSet(&Glob), *Deser.getPointsToSet(&Glob));
  }
  for (const auto &Fun : Mod) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      EXPECT_EQ(*PTS.getPointsToSet(&Inst), *Deser.getPointsToSet(&Inst));
    }
  }
}

static void analyze(llvm::StringRef File, const GroundTruthTy &Gt,
                    llvm::StringRef EntryPoint = "main") {
  Logger::disable();
  ValueAnnotationPass::resetValueID();
  ProjectIRDB IRDB({"llvm_test_code/" + File.str()});

  // llvm::outs() << *IRDB.getWPAModule() << '\n';

  LLVMPointsToSet PTS(IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(IRDB, CallGraphAnalysisType::OTF, {EntryPoint.str()}, &TH,
                    &PTS);

  auto Ser = PTS.getAsJson();
  checkSer(Ser, Gt);

  LLVMPointsToSet Deser(IRDB, Ser);
  checkDeser(*IRDB.getWPAModule(), PTS, Deser);
}

TEST(LLVMPointsToSetSerializationTest, Ser_Intra01) {
  analyze("pointers/basic_01.ll", {{{"1"}, {"0", "3"}}, {"main"}});
}

TEST(LLVMPointsToSetSerializationTest, Ser_Inter01) {
  analyze("pointers/call_01.ll",
          {{{"0"}, {"10", "12", "2", "6", "_Z10setIntegerPi.0"}, {"5"}, {"7"}},
           {"main", "_Z10setIntegerPi"}});
}

TEST(LLVMPointsToSetSerializationTest, Ser_Global01) {
  analyze("pointers/global_01.ll",
          {{{"0", "15", "17", "2", "3", "9", "_Z3fooPi.0"},
            {"1"},
            {"12"},
            {"13"},
            {"7"}},
           {"_GLOBAL__sub_I_global_01.cpp", "_Z3fooPi", "__cxx_global_var_init",
            "main"}});
}
