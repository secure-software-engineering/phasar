#include "phasar/PhasarLLVM/Utils/TaintConfiguration.h"
#include "phasar/Config/Configuration.h"

#include "llvm/ADT/StringRef.h"

#include "gtest/gtest.h"

#include "TestConfig.h"

using namespace psr;

TEST(TaintConfigurationTest, HandleSSImport) {
  TaintConfiguration<int> TSF("config/phasar-source-sink-function.json");
  std::cout << TSF;
  EXPECT_EQ(TaintConfiguration<int>::SourceFunction(
                "read", false, std::vector<unsigned>({0, 1, 3})),
            TSF.getSource("read"));
  EXPECT_EQ(TaintConfiguration<int>::SinkFunction("write",
                                                  std::vector<unsigned>({1})),
            TSF.getSink("write"));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
