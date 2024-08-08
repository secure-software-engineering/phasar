#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

namespace {

using llvm::demangle;

static const llvm::StructType *getType(const LLVMProjectIRDB &IRDB,
                                       llvm::StringRef Name) {
  // TODO: Optimize
  for (const auto *Ty : IRDB.getModule()->getIdentifiedStructTypes()) {
    if (Ty->getName() == Name) {
      return Ty;
    }
  }
  return nullptr;
}

// check if the vtables are constructed correctly in more complex scenarios

TEST(VTableTest, VTableConstruction_01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_1_cpp.ll"});
  LLVMVFTableProvider TH(IRDB);
  // TODO
}

TEST(VTableTest, VTableConstruction_02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_7_cpp.ll"});
  LLVMVFTableProvider TH(IRDB);
  // TODO
}

TEST(VTableTest, VTableConstruction_03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_8_cpp.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Base")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Child")));

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Base::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_04) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_9_cpp.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Base")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Child")));

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Base::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_05) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_10_cpp.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Base")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "struct.Child")));

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "__cxa_pure_virtual");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "struct.Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_6) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_14_cpp.ll"});

  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "class.Base")));
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "class.Base"))->size(), 3U);
}
} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
