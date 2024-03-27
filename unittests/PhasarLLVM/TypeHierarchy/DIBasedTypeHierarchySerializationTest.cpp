
#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

class LLVMBasedICFGGSerializationTest : public ::testing::Test {
  void serAndDeser(const llvm::Twine &IRFile) {
    using namespace std::string_literals;

    /// TODO: add all IRFiles in an array and run tests over these files

    psr::LLVMProjectIRDB IRDB(IRFile);
    psr::DIBasedTypeHierarchy DIBTH(IRDB);

    std::string Ser;
    // stream data into a json file using the printAsJson() function
    llvm::raw_string_ostream StringStream(Ser);

    DIBTH.printAsJson(StringStream);

    psr::DIBasedTypeHierarchy DeserializedDIBTH(
        &IRDB, psr::DIBasedTypeHierarchyData::loadJsonString(Ser));

    compareResults(DIBTH, DeserializedDIBTH);
  }

  void compareResults(const psr::DIBasedTypeHierarchy &Orig,
                      const psr::DIBasedTypeHierarchy &Deser) {

    ASSERT_EQ(Orig.getAllTypes().size(), Deser.getAllTypes().size());
    ASSERT_EQ(Orig.getAllVTables().size(), Deser.getAllVTables().size());

    for (const auto &OrigCurrentType : Orig.getAllTypes()) {
      EXPECT_EQ(OrigCurrentType,
                Deser.getType(OrigCurrentType->getName().str()));
      EXPECT_EQ(Orig.getVFTable(OrigCurrentType),
                Deser.getVFTable(OrigCurrentType));
    }
  }

  int main(int Argc, char **Argv) {
    ::testing::InitGoogleTest(&Argc, Argv);
    auto Res = RUN_ALL_TESTS();
    return Res;
  }
};