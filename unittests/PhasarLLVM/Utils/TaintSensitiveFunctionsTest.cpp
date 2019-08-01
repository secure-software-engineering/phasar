#include <gtest/gtest.h>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>

using namespace psr;

TEST(TaintConfigurationTest, HandleSSImport) {
  TaintConfiguration TSF;
  TSF.importSourceSinkFunctions();
  std::cout << TSF;
  std::map<std::string, TaintConfiguration::SourceFunction> TSources;
  std::map<std::string, TaintConfiguration::SinkFunction> TSinks;
  TSources.insert(std::make_pair(
      "source()", TaintConfiguration::SourceFunction("source()", true)));
  TSources.insert(std::make_pair(
      "read", TaintConfiguration::SourceFunction("read", {0, 1, 3}, false)));
  TSinks.insert(std::make_pair(
      "sink(int)", TaintConfiguration::SinkFunction("sink(int)", {0})));
  TSinks.insert(
      std::make_pair("write", TaintConfiguration::SinkFunction("write", {1})));
  EXPECT_EQ(TSF.Sources, TSources);
  EXPECT_EQ(TSF.Sinks, TSinks);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}