#include "phasar/Utils/Logger.h"

#include "gtest/gtest.h"

TEST(LoggerTest, BasicWithinPsr) {
  using namespace psr;
  Logger::initializeStderrLogger(SeverityLevel::INFO);
  Logger::initializeStderrLogger(SeverityLevel::DEBUG, "LLVMAliasSet");
  PHASAR_LOG("This should not be shown");
  PHASAR_LOG_LEVEL(INFO, "Basic category-less message");
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMAliasSet",
                       "Some message specific to LLVMAliasSet");
}

TEST(LoggerTest, BasicOutsidePsr) {
  psr::Logger::initializeStderrLogger(psr::SeverityLevel::INFO);
  psr::Logger::initializeStderrLogger(psr::SeverityLevel::DEBUG,
                                      "LLVMAliasSet");
  PHASAR_LOG("This should not be shown");
  PHASAR_LOG_LEVEL(INFO, "Basic category-less message");
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMAliasSet",
                       "Some message specific to LLVMAliasSet");
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
