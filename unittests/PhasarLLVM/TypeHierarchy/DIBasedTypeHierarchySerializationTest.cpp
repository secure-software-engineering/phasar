
#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class TypeHierarchySerialization
    : public ::testing::TestWithParam<std::string_view> {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("type_hierarchies/");
  const std::vector<std::string> EntryPoints = {"main"};

}; // Test Fixture

void compareResults(const psr::DIBasedTypeHierarchy &Orig,
                    const psr::DIBasedTypeHierarchy &Deser) {
  ASSERT_EQ(Orig.getAllTypes().size(), Deser.getAllTypes().size());
  ASSERT_EQ(Orig.getAllVTables().size(), Deser.getAllVTables().size());

  for (const auto &OrigCurrentType : Orig.getAllTypes()) {
    EXPECT_EQ(OrigCurrentType, Deser.getType(OrigCurrentType->getName().str()));
    EXPECT_EQ(Orig.getVFTable(OrigCurrentType),
              Deser.getVFTable(OrigCurrentType));
  }
}

TEST_P(TypeHierarchySerialization, OrigAndDeserEqual) {
  using namespace std::string_literals;

  psr::LLVMProjectIRDB IRDB(psr::unittest::PathToLLTestFiles + GetParam());
  psr::DIBasedTypeHierarchy DIBTH(IRDB);

  std::string Ser;
  // stream data into a json file using the printAsJson() function
  llvm::raw_string_ostream StringStream(Ser);

  DIBTH.printAsJson(StringStream);

  psr::DIBasedTypeHierarchy DeserializedDIBTH(
      &IRDB, psr::DIBasedTypeHierarchyData::loadJsonString(Ser));

  compareResults(DIBTH, DeserializedDIBTH);
}

static constexpr std::string_view TypeHierarchyTestFiles[] = {
    "type_hierarchy_1_cpp_dbg.ll", "type_hierarchy_2_cpp_dbg.ll"};

INSTANTIATE_TEST_SUITE_P(TypeHierarchySerializationTest,
                         TypeHierarchySerialization,
                         ::testing::ValuesIn(TypeHierarchyTestFiles));

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
