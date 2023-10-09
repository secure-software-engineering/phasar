
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include "../TestUtils/TestConfig.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <string>

//===----------------------------------------------------------------------===//
// Unit tests for the code annotation taint configuration

static constexpr auto PathToAttrTaintConfigTestCode =
    PHASAR_BUILD_SUBFOLDER("TaintConfig/AttrConfig/");

namespace {
class TaintConfigTest : public ::testing::Test {

public:
  void SetUp() override { psr::ValueAnnotationPass::resetValueID(); }

  std::string getFunctionName(std::string MangledFunctionName) {
    if (MangledFunctionName.find('(') != std::string::npos) {
      return MangledFunctionName.substr(0, MangledFunctionName.find('('));
    }
    return MangledFunctionName;
  }
};

} // anonymous namespace

TEST_F(TaintConfigTest, Array_01) {
  const std::string File = "array_01_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  const llvm::Value *I = IR.getInstruction(5);
  ASSERT_TRUE(Config.isSource(I));
}

TEST_F(TaintConfigTest, Array_02) {
  const std::string File = "array_02_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const llvm::Value *I = IR.getInstruction(5);
  ASSERT_TRUE(Config.isSource(I));
}

TEST_F(TaintConfigTest, Basic_01) {
  const std::string File = "basic_01_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const auto *Bar = IR.getFunction("bar");
  assert(Bar);
  for (const auto &User : Bar->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(Config.isSource(User));
    }
  }
  const auto *Baz = IR.getFunction("baz");
  assert(Baz);
  ASSERT_TRUE(Config.isSink(Baz->getArg(1)));
  const auto *Foo = IR.getFunction("foo");
  assert(Foo);
  ASSERT_TRUE(Config.isSink(Foo->getArg(1)));
}

TEST_F(TaintConfigTest, Basic_02) {
  const std::string File = "basic_02_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const llvm::Value *I1 = IR.getInstruction(9);
  const llvm::Value *I2 = IR.getInstruction(23);
  ASSERT_TRUE(Config.isSource(I1));
  ASSERT_TRUE(Config.isSource(I2));
}

TEST_F(TaintConfigTest, Basic_03) {
  const std::string File = "basic_03_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const auto *TaintPair = IR.getFunction("taintPair");
  assert(TaintPair);
  for (const auto &User : TaintPair->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(Config.isSource(User));
    }
  }
}

TEST_F(TaintConfigTest, Basic_04) {
  const std::string File = "basic_04_c_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const llvm::Value *I = IR.getInstruction(4);
  ASSERT_TRUE(Config.isSource(I));
}

TEST_F(TaintConfigTest, DataMember_01) {
  const std::string File = "data_member_01_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const llvm::Value *I = IR.getInstruction(9);
  ASSERT_TRUE(Config.isSource(I));
}

TEST_F(TaintConfigTest, FunMember_01) {
  const std::string File = "fun_member_01_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR);
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  llvm::outs() << TConfig << '\n';
  for (const auto &F : IR.getAllFunctions()) {
    if (F->getName().contains("foo")) {
      assert(F);
      for (const auto &User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(TConfig.isSource(User));
        }
      }
    } else if (F->getName().contains("bar")) {
      assert(F);
      ASSERT_TRUE(TConfig.isSink(F->getArg(1)));
    }
  }
}

TEST_F(TaintConfigTest, FunMember_02) {
  const std::string File = "fun_member_02_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR);
  // IR.emitPreprocessedIR(llvm::outs(), false);
  llvm::outs() << TConfig << '\n';
  const llvm::Value *I1 = IR.getInstruction(22);
  const llvm::Value *I2 = IR.getInstruction(61);
  const llvm::Value *I3 = IR.getInstruction(73);
  const llvm::Value *I4 = IR.getInstruction(84);
  ASSERT_TRUE(TConfig.isSource(I1));
  ASSERT_TRUE(TConfig.isSource(I2));
  ASSERT_TRUE(TConfig.isSource(I3));
  ASSERT_TRUE(TConfig.isSource(I4));
  // TODO sink annotation is missing for destructor
  const auto *Sanitizer = IR.getFunction("_ZN1X5sanitEv");
  assert(Sanitizer);
  for (const auto &User : Sanitizer->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(TConfig.isSanitizer(User));
    }
  }
}

TEST_F(TaintConfigTest, NameMangling_01) {
  const std::string File = "name_mangling_01_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "foo") {
      assert(F);
      if (F->arg_size() == 2) {
        ASSERT_TRUE(Config.isSource(F->getArg(0)));
      } else if (F->arg_size() == 4) {
        ASSERT_TRUE(Config.isSource(F->getArg(3)));
      }
    }
  }
}

TEST_F(TaintConfigTest, StaticFun_01) {
  const std::string File = "static_fun_01_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "bar") {
      assert(F);
      for (const auto &User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(Config.isSource(User));
        }
      }
      break;
    }
  }
}

TEST_F(TaintConfigTest, StaticFun_02) {
  const std::string File = "static_fun_02_cpp_dbg.ll";
  psr::LLVMProjectIRDB IR({PathToAttrTaintConfigTestCode + File});
  psr::LLVMTaintConfig Config(IR);
  llvm::outs() << Config << '\n';
  const llvm::Value *CallInst = IR.getInstruction(16);
  const auto *I = llvm::dyn_cast<llvm::CallBase>(CallInst);
  ASSERT_TRUE(I && Config.isSource(I->getCalledFunction()->getArg(0)));
  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "bar") {
      assert(F);
      for (const auto &User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(Config.isSource(User));
        }
      }
    }
  }
}

//===----------------------------------------------------------------------===//
// Unit tests for the json taint configuration

static constexpr auto PathToJsonTaintConfigTestCode =
    PHASAR_BUILD_SUBFOLDER("TaintConfig/JsonConfig/");

TEST_F(TaintConfigTest, Array_01_Json) {
  const std::string File = "array_01_c_dbg.ll";
  const std::string Config = "array_01_config.json";

  auto JsonConfig =
      psr::parseTaintConfig({PathToJsonTaintConfigTestCode.str() + Config});

  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});

  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);

  const llvm::Value *I = IR.getInstruction(3);
  ASSERT_TRUE(TConfig.isSource(I));
}

TEST_F(TaintConfigTest, Array_02_Json) {
  const std::string File = "array_02_c_dbg.ll";
  const std::string Config = "array_02_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  const llvm::Value *I = IR.getInstruction(3);
  ASSERT_TRUE(TConfig.isSource(I));
}

TEST_F(TaintConfigTest, Basic_01_Json) {
  const std::string File = "basic_01_c_dbg.ll";
  const std::string Config = "basic_01_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);

  const auto *Bar = IR.getFunction("bar");
  assert(Bar);
  for (const auto &User : Bar->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(TConfig.isSource(User));
    }
  }
  const auto *Baz = IR.getFunction("baz");
  assert(Baz);
  ASSERT_TRUE(TConfig.isSink(Baz->getArg(1)));
  const auto *Foo = IR.getFunction("foo");
  assert(Foo);
  ASSERT_TRUE(TConfig.isSink(Foo->getArg(1)));
}

TEST_F(TaintConfigTest, Basic_02_Json) {
  const std::string File = "basic_02_c_dbg.ll";
  const std::string Config = "basic_02_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  const llvm::Value *I1 = IR.getInstruction(7);
  const llvm::Value *I2 = IR.getInstruction(18);
  ASSERT_TRUE(TConfig.isSource(I1));
  ASSERT_TRUE(TConfig.isSource(I2));
}

TEST_F(TaintConfigTest, Basic_03_Json) {
  const std::string File = "basic_03_c_dbg.ll";
  const std::string Config = "basic_03_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  llvm::outs() << TConfig << '\n';
  const auto *TaintPair = IR.getFunction("taintPair");
  assert(TaintPair);
  for (const auto &User : TaintPair->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(TConfig.isSource(User));
    }
  }
}

TEST_F(TaintConfigTest, Basic_04_Json) {
  const std::string File = "basic_04_c_dbg.ll";
  const std::string Config = "basic_04_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  const llvm::Value *I = IR.getInstruction(2);
  ASSERT_TRUE(TConfig.isSource(I));
}

TEST_F(TaintConfigTest, DataMember_01_Json) {
  const std::string File = "data_member_01_cpp_dbg.ll";
  const std::string Config = "data_member_01_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  const llvm::Value *I = IR.getInstruction(17);
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  ASSERT_TRUE(TConfig.isSource(I));
}

TEST_F(TaintConfigTest, FunMember_01_Json) {
  const std::string File = "fun_member_01_cpp_dbg.ll";
  const std::string Config = "fun_member_01_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  llvm::outs() << TConfig << '\n';
  for (const auto *F : IR.getAllFunctions()) {
    if (F->getName().contains("foo")) {
      assert(F);
      for (const auto *User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(TConfig.isSource(User));
        }
      }
    } else if (F->getName().contains("bar")) {
      assert(F);
      ASSERT_TRUE(TConfig.isSink(F->getArg(0)));
    }
  }
}

TEST_F(TaintConfigTest, FunMember_02_Json) {
  const std::string File = "fun_member_02_cpp_dbg.ll";
  const std::string Config = "fun_member_02_config.json";

  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);

  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  //   IR.emitPreprocessedIR(llvm::outs(), false);
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  const llvm::Value *I1 = IR.getInstruction(18);
  const llvm::Value *I2 = IR.getInstruction(54);
  const llvm::Value *I3 = IR.getInstruction(63);
  const llvm::Value *I4 = IR.getInstruction(71);
  ASSERT_TRUE(TConfig.isSource(I1));
  ASSERT_TRUE(TConfig.isSource(I2));
  ASSERT_TRUE(TConfig.isSource(I3));
  ASSERT_TRUE(TConfig.isSource(I4));

  const auto *DestructorX = IR.getFunction("_ZN1XD2Ev");
  assert(DestructorX);
  for (const auto *Arg = DestructorX->arg_begin();
       Arg != DestructorX->arg_end(); ++Arg) {
    ASSERT_TRUE(TConfig.isSink(Arg));
  }

  const auto *Sanitizer = IR.getFunction("_ZN1X5sanitEv");
  assert(Sanitizer);
  for (const auto &User : Sanitizer->users()) {
    if (llvm::isa<llvm::CallBase>(User)) {
      ASSERT_TRUE(TConfig.isSanitizer(User));
    }
  }
}

TEST_F(TaintConfigTest, NameMangling_01_Json) {
  const std::string File = "name_mangling_01_cpp_dbg.ll";
  const std::string Config = "name_mangling_01_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);

  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "foo") {
      assert(F);
      if (F->arg_size() == 2) {
        ASSERT_TRUE(TConfig.isSource(F->getArg(0)));
      } else if (F->arg_size() == 4) {
        ASSERT_TRUE(TConfig.isSource(F->getArg(3)));
      }
    }
  }
}

TEST_F(TaintConfigTest, StaticFun_01_Json) {
  const std::string File = "static_fun_01_cpp_dbg.ll";
  const std::string Config = "static_fun_01_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  llvm::outs() << TConfig << '\n';
  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "bar") {
      assert(F);
      for (const auto &User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(TConfig.isSource(User));
        }
      }
      break;
    }
  }
}

TEST_F(TaintConfigTest, StaticFun_02_Json) {
  const std::string File = "static_fun_02_cpp_dbg.ll";
  const std::string Config = "static_fun_02_config.json";
  auto JsonConfig =
      psr::parseTaintConfig(PathToJsonTaintConfigTestCode + Config);
  psr::LLVMProjectIRDB IR({PathToJsonTaintConfigTestCode + File});
  psr::LLVMTaintConfig TConfig(IR, JsonConfig);
  llvm::outs() << TConfig << '\n';
  const llvm::Value *CallInst = IR.getInstruction(13);
  const auto *I = llvm::dyn_cast<llvm::CallBase>(CallInst);
  ASSERT_TRUE(I && TConfig.isSource(I->getCalledFunction()->getArg(0)));
  for (const auto *F : IR.getAllFunctions()) {
    std::string FName = getFunctionName(llvm::demangle(F->getName().str()));
    if (FName == "bar") {
      assert(F);
      for (const auto &User : F->users()) {
        if (llvm::isa<llvm::CallBase>(User)) {
          ASSERT_TRUE(TConfig.isSource(User));
        }
      }
    } else if (FName == "foo" || F->getName().contains("foo")) {
      ASSERT_TRUE(TConfig.isSource(F->getArg(0)));
    }
  }
}

//===----------------------------------------------------------------------===//
// The unit tests' entry point

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
