#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <cstdlib>

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

  ASSERT_TRUE(Ser.count("AliasSets"));
  ASSERT_TRUE(Ser.count("AnalyzedFunctions"));

  const auto &PSets = Ser.at("AliasSets");
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

static void checkDeser(const llvm::Module &Mod, LLVMAliasSet &PTS,
                       LLVMAliasSet &Deser) {
  for (const auto &Glob : Mod.globals()) {
    EXPECT_EQ(*PTS.getAliasSet(&Glob), *Deser.getAliasSet(&Glob));
  }
  for (const auto &Fun : Mod) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      EXPECT_EQ(*PTS.getAliasSet(&Inst), *Deser.getAliasSet(&Inst));
    }
  }
}

static void analyze(llvm::StringRef File, const GroundTruthTy &Gt,
                    llvm::StringRef EntryPoint = "main") {
  Logger::disable();
  ValueAnnotationPass::resetValueID();
  LLVMProjectIRDB IRDB({"llvm_test_code/" + File.str()});

  // llvm::outs() << *IRDB.getWPAModule() << '\n';

  LLVMAliasSet PTS(&IRDB, false);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICF(&IRDB, CallGraphAnalysisType::OTF, {EntryPoint.str()}, &TH,
                    &PTS);

  auto Ser = PTS.getAsJson();
  checkSer(Ser, Gt);

  LLVMAliasSet Deser(&IRDB, Ser);
  checkDeser(*IRDB.getModule(), PTS, Deser);
}

TEST(LLVMAliasSetSerializationTest, Ser_Intra01) {
  analyze("pointers/basic_01.ll", {{{"1"}, {"0", "3"}}, {"main"}});
}

TEST(LLVMAliasSetSerializationTest, Ser_Inter01) {
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
