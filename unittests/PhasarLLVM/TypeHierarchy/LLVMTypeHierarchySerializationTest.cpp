
#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/NlohmannLogging.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DerivedTypes.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class LLVMTypeHierarchySerialization
    : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("type_hierarchies/");
  const std::vector<std::string> EntryPoints = {"main"};

}; // Test Fixture

void compareResults(psr::LLVMTypeHierarchy &Orig,
                    psr::LLVMTypeHierarchy &Deser) {
  ASSERT_EQ(Orig.getAllTypes().size(), Deser.getAllTypes().size());

  for (const auto &OrigCurrentType : Orig.getAllTypes()) {
    // check types
    EXPECT_EQ(OrigCurrentType, Deser.getType(OrigCurrentType->getName().str()));

    // check edges
    for (const auto &OrigEdgeCurrentType : Orig.getAllTypes()) {
      // Deser.isSubType can take the same arguments as Orig.isSubType, since
      // Deser should have the same types

      bool ExpectedValue = Orig.isSubType(OrigCurrentType, OrigEdgeCurrentType);
      bool DeserializedValue =
          Deser.isSubType(Deser.getType(OrigCurrentType->getName().str()),
                          Deser.getType(OrigEdgeCurrentType->getName().str()));

      EXPECT_EQ(ExpectedValue, DeserializedValue);
    }
  }
}

TEST_P(LLVMTypeHierarchySerialization, OrigAndDeserEqual) {
  using namespace std::string_literals;

  psr::LLVMProjectIRDB IRDB(PathToLlFiles + GetParam());
  psr::LLVMTypeHierarchy TypeHierarchy(IRDB);

  std::string Ser;
  llvm::raw_string_ostream StringStream(Ser);

  TypeHierarchy.printAsJson(StringStream);

  psr::LLVMTypeHierarchy DeserializedTypeHierarchy(
      IRDB, psr::LLVMTypeHierarchyData::loadJsonString(Ser));

  compareResults(TypeHierarchy, DeserializedTypeHierarchy);
}

static constexpr std::string_view TypeHierarchyTestFiles[] = {
    "type_hierarchy_1_cpp_dbg.ll",    "type_hierarchy_2_cpp_dbg.ll",
    "type_hierarchy_3_cpp_dbg.ll",    "type_hierarchy_4_cpp_dbg.ll",
    "type_hierarchy_5_cpp_dbg.ll",    "type_hierarchy_6_cpp_dbg.ll",
    "type_hierarchy_7_cpp_dbg.ll",    "type_hierarchy_7_b_cpp_dbg.ll",
    "type_hierarchy_8_cpp_dbg.ll",    "type_hierarchy_9_cpp_dbg.ll",
    "type_hierarchy_10_cpp_dbg.ll",   "type_hierarchy_11_cpp_dbg.ll",
    "type_hierarchy_12_cpp_dbg.ll",   "type_hierarchy_12_b_cpp_dbg.ll",
    "type_hierarchy_12_c_cpp_dbg.ll", "type_hierarchy_13_cpp_dbg.ll",
    "type_hierarchy_14_cpp_dbg.ll",   "type_hierarchy_15_cpp_dbg.ll",
    "type_hierarchy_16_cpp_dbg.ll",   "type_hierarchy_17_cpp_dbg.ll",
    "type_hierarchy_18_cpp_dbg.ll",   "type_hierarchy_19_cpp_dbg.ll",
    "type_hierarchy_20_cpp_dbg.ll",   "type_hierarchy_21_cpp_dbg.ll",
};

INSTANTIATE_TEST_SUITE_P(LLVMTypeHierarchySerializationTest,
                         LLVMTypeHierarchySerialization,
                         ::testing::ValuesIn(TypeHierarchyTestFiles));

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
