#include "phasar/PhasarLLVM/Utils/TaintConfiguration.h"
#include "gtest/gtest.h"

using namespace psr;

TEST(TaintConfigurationTest, HandleSSImport) {
  TaintConfiguration<int> TSF(PhasarConfig::DefaultSourceSinkFunctionsPath());
  std::cout << TSF;
  EXPECT_EQ(TaintConfiguration<int>::SourceFunction(
                "read", false, std::vector<unsigned>({0, 1, 3})),
            TSF.getSource("read"));
  EXPECT_EQ(TaintConfiguration<int>::SinkFunction("write",
                                                  std::vector<unsigned>({1})),
            TSF.getSink("write"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
