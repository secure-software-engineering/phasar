
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

    for (const auto &CurrVFunc :
         Orig.getVFTable(OrigCurrentType)->getAllFunctions()) {
      bool FoundFunction = false;
      for (const auto &DeserFunc :
           Deser.getVFTable(OrigCurrentType)->getAllFunctions()) {
        if (DeserFunc) {
          if (CurrVFunc) {
            if (DeserFunc->getName() == CurrVFunc->getName()) {
              FoundFunction = true;
              break;
            }
          }
        }
      }
      EXPECT_TRUE(FoundFunction);
    }
  }
}

TEST_P(TypeHierarchySerialization, OrigAndDeserEqual) {
  using namespace std::string_literals;

  psr::LLVMProjectIRDB IRDB(PathToLlFiles + GetParam());
  psr::DIBasedTypeHierarchy DIBTH(IRDB);

  std::string Ser;
  llvm::raw_string_ostream StringStream(Ser);

  DIBTH.printAsJson(StringStream);

  psr::DIBasedTypeHierarchy DeserializedDIBTH(
      &IRDB, psr::DIBasedTypeHierarchyData::loadJsonString(Ser));

  compareResults(DIBTH, DeserializedDIBTH);
  /*
    DIBTH.printAsJson();
    llvm::outs() << "\n----------------------------\n";
    llvm::outs() << Ser << "\n----------------------------\n";
    llvm::outs().flush();
  */
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

INSTANTIATE_TEST_SUITE_P(TypeHierarchySerializationTest,
                         TypeHierarchySerialization,
                         ::testing::ValuesIn(TypeHierarchyTestFiles));

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
