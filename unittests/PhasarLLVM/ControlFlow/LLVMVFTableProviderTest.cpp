#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

namespace {

using llvm::demangle;

static const llvm::DIType *getType(const LLVMProjectIRDB &IRDB,
                                   llvm::StringRef Name) {
  // TODO: Optimize
  for (const auto *Instr : IRDB.getAllInstructions()) {
    if (const auto *Val = llvm::dyn_cast<llvm::Value>(Instr)) {
      if (const auto *DILocVal = getDILocalVariable(Val)) {
        // case: is DIDerivedType
        if (const auto *DerivedTy =
                llvm::dyn_cast<llvm::DIDerivedType>(DILocVal->getType())) {
          if (const auto *DITy = DerivedTy->getBaseType()) {
            if (DITy->getName() == Name) {
              return DITy;
            }
          }
        }
        // case: isn't DIDerivedType
        if (const auto *DITy = DILocVal->getType()) {
          if (DITy->getName() == Name) {
            return DITy;
          }
        }
      }
    }
  }
  return nullptr;
}

// check if the vtables are constructed correctly in more complex scenarios

TEST(VTableTest, VTableConstruction_01) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_1_cpp_dbg.ll"});
  LLVMVFTableProvider TH(IRDB);
  // TODO
}

TEST(VTableTest, VTableConstruction_02) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_7_cpp_dbg.ll"});
  LLVMVFTableProvider TH(IRDB);
  // TODO
}

TEST(VTableTest, VTableConstruction_03) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_8_cpp_dbg.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Base")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Child")));

  llvm::outs() << "AllFuncs:\n";
  for (const auto *CurrFunc :
       TH.getVFTableOrNull(getType(IRDB, "Base"))->getAllFunctions()) {
    llvm::outs() << CurrFunc << "\n";
  }

  llvm::outs() << "getFunction(0)\n"
               << TH.getVFTableOrNull(getType(IRDB, "Base"))->getFunction(0)
               << "\n";

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Base::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_04) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_9_cpp_dbg.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Base")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Child")));

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Base::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_05) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_10_cpp_dbg.ll"});
  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Child")));
  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Child")));

  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "__cxa_pure_virtual");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Base"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Base"))->size(), 2U);
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(0)
                         ->getName()
                         .str()),
            "Child::foo()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(1)
                         ->getName()
                         .str()),
            "Base::bar()");
  EXPECT_EQ(demangle(TH.getVFTableOrNull(getType(IRDB, "Child"))
                         ->getFunction(2)
                         ->getName()
                         .str()),
            "Child::baz()");
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Child"))->size(), 3U);
}

TEST(VTableTest, VTableConstruction_6) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_14_cpp_dbg.ll"});

  LLVMVFTableProvider TH(IRDB);

  ASSERT_TRUE(TH.hasVFTable(getType(IRDB, "Base")));
  EXPECT_EQ(TH.getVFTableOrNull(getType(IRDB, "Base"))->size(), 3U);
}
} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
