
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

class LLVMBasedICFGGSerializationTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("call_graphs/");

  void serAndDeser(const llvm::Twine &IRFile) {
    using namespace std::string_literals;

    psr::LLVMProjectIRDB IRDB(PathToLLFiles + IRFile);

    psr::LLVMBasedICFG ICF(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"s});
    auto Ser = ICF.getAsJson();

    psr::LLVMBasedICFG Deser(&IRDB, Ser);

    compareResults(ICF, Deser);
  }

  void compareResults(const psr::LLVMBasedICFG &Orig,
                      const psr::LLVMBasedICFG &Deser) {

    EXPECT_EQ(Orig.getNumVertexFunctions(), Deser.getNumVertexFunctions());

    {
      llvm::DenseSet<const llvm::Function *> DeserFuns(
          Deser.getAllVertexFunctions().begin(),
          Deser.getAllVertexFunctions().end());
      for (const auto *Fun : Orig.getAllVertexFunctions()) {
        EXPECT_TRUE(DeserFuns.contains(Fun))
            << "Deserialized ICFG does not contain vertex function "
            << Fun->getName().str();
      }
    }

    for (const auto *Fun : Orig.getAllVertexFunctions()) {

      const auto &Calls = Orig.getCallsFromWithin(Fun);

      for (const auto *CS : Calls) {
        llvm::DenseSet<const llvm::Function *> DeserCallees(
            Deser.getCalleesOfCallAt(CS).begin(),
            Deser.getCalleesOfCallAt(CS).end());
        EXPECT_EQ(Orig.getCalleesOfCallAt(CS).size(), DeserCallees.size());

        for (const auto *OrigCallee : Orig.getCalleesOfCallAt(CS)) {
          EXPECT_TRUE(DeserCallees.contains(OrigCallee))
              << "Deserialized ICFG does not contain call to "
              << OrigCallee->getName().str() << " from "
              << psr::llvmIRToString(CS);
        }
      }
    }
  }
};

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG01) {
  serAndDeser("static_callsite_1.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG02) {
  serAndDeser("static_callsite_2.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG03) {
  serAndDeser("static_callsite_3.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG04) {
  serAndDeser("static_callsite_4.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG05) {
  serAndDeser("static_callsite_5.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG06) {
  serAndDeser("static_callsite_6.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG07) {
  serAndDeser("static_callsite_7.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG08) {
  serAndDeser("static_callsite_8.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG09) {
  serAndDeser("static_callsite_9.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG10) {
  serAndDeser("static_callsite_10.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG11) {
  serAndDeser("static_callsite_11.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG12) {
  serAndDeser("static_callsite_12.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFG13) {
  serAndDeser("static_callsite_13.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV1) {
  serAndDeser("virtual_call_1.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV2) {
  serAndDeser("virtual_call_2.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV3) {
  serAndDeser("virtual_call_3.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV4) {
  serAndDeser("virtual_call_4.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV5) {
  serAndDeser("virtual_call_5.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV6) {
  serAndDeser("virtual_call_6.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV7) {
  serAndDeser("virtual_call_7.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV8) {
  serAndDeser("virtual_call_8.ll");
}

TEST_F(LLVMBasedICFGGSerializationTest, SerICFGV9) {
  serAndDeser("virtual_call_9.ll");
}
