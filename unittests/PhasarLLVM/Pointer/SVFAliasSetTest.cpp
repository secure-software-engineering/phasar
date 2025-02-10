
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

// TEST(SVFAliasSetTest, Intra_01) {
//   LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
//                        "pointers/basic_01_cpp_dbg.ll");

//   SVFAliasSet AS(&IRDB);

//   const auto *V = IRDB.getInstruction(5);
//   ASSERT_TRUE(V && V->getType()->isPointerTy());

//   // auto Pts = AS.getAliasSet(V);
// }

// TEST(SVFAliasSetTest, Call_01) {
//   LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
//                        "pointers/call_01_cpp_dbg.ll");

//   SVFAliasSet AS(&IRDB);

//   const auto *V = IRDB.getInstruction(15);
//   ASSERT_TRUE(V && V->getType()->isPointerTy());

//   auto Pts = AS.getAliasSet(V);
// }

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
