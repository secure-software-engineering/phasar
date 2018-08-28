#include <gtest/gtest.h>
#include <phasar/PhasarLLVM/Utils/TaintSensitiveFunctions.h>

using namespace psr;

TEST(TaintSensitiveFunctionsTest, HandleSSImport) {
  TaintSensitiveFunctions TSF;
  TSF.importSourceSinkFunctions();
  std::cout << TSF;
  std::map<std::string, TaintSensitiveFunctions::SourceFunction> TSources;
  std::map<std::string, TaintSensitiveFunctions::SinkFunction> TSinks;
  TSources.insert(std::make_pair(
      "source()", TaintSensitiveFunctions::SourceFunction("source()", true)));
  TSources.insert(std::make_pair(
      "read",
      TaintSensitiveFunctions::SourceFunction("read", {0, 1, 3}, false)));
  TSinks.insert(std::make_pair(
      "sink(int)", TaintSensitiveFunctions::SinkFunction("sink(int)", {0})));
  TSinks.insert(std::make_pair(
      "write", TaintSensitiveFunctions::SinkFunction("write", {1})));
  EXPECT_EQ(TSF.Sources, TSources);
  EXPECT_EQ(TSF.Sinks, TSinks);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}