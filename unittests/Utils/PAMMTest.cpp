#include "phasar/Utils/PAMM.h"

#include "phasar/Config/Configuration.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <thread>

using namespace psr;

/* Test fixture */
class PAMMTest : public ::testing::Test {
protected:
  using time_point = std::chrono::high_resolution_clock::time_point;
  PAMMTest() = default;
  ~PAMMTest() override = default;

  void SetUp() override {}

  void TearDown() override {
    PAMM &Pamm = PAMM::getInstance();
    Pamm.printMeasuredData(llvm::outs());
    Pamm.reset();
  }
};

TEST_F(PAMMTest, HandleTimer) {
  //   PAMM &pamm = PAMM::getInstance();
  PAMM::getInstance().startTimer("timer1");
  std::this_thread::sleep_for(std::chrono::milliseconds(120));
  PAMM::getInstance().stopTimer("timer1");
  auto Elapsed = PAMM::getInstance().elapsedTime("timer1");
  EXPECT_GE(Elapsed, 120U) << "Bad time measurement";
  EXPECT_LT(Elapsed, 240U) << "Too much tolerance";
}

TEST_F(PAMMTest, HandleCounter) {
  PAMM &Pamm = PAMM::getInstance();
  Pamm.regCounter("first");
  Pamm.regCounter("second");
  Pamm.regCounter("third");
  Pamm.incCounter("first", 42);
  Pamm.incCounter("second");
  Pamm.incCounter("second");
  Pamm.incCounter("third", 2);
  Pamm.decCounter("third", 2);
  EXPECT_EQ(Pamm.getCounter("first"), 42);
  EXPECT_EQ(Pamm.getCounter("second"), 2);
  EXPECT_EQ(Pamm.getCounter("third"), 0);
}

TEST_F(PAMMTest, HandleJSONOutput) {
  PAMM &Pamm = PAMM::getInstance();
  Pamm.regCounter("timerCount");
  Pamm.regCounter("setOpCount");
  Pamm.startTimer("timer1");
  Pamm.incCounter("timerCount");
  Pamm.startTimer("timer2");
  Pamm.incCounter("timerCount");
  Pamm.regHistogram("Test-Set");
  Pamm.addToHistogram("Test-Set", "13");
  Pamm.addToHistogram("Test-Set", "13");
  Pamm.addToHistogram("Test-Set", "13");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "54");
  Pamm.addToHistogram("Test-Set", "54");
  Pamm.addToHistogram("Test-Set", "54");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.incCounter("setOpCount", 11);

  std::this_thread::sleep_for(std::chrono::milliseconds(180));
  Pamm.stopTimer("timer2");

  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "42");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.incCounter("setOpCount", 10);

  Pamm.startTimer("timer3");
  Pamm.incCounter("timerCount");
  std::this_thread::sleep_for(std::chrono::milliseconds(230));

  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "1");
  Pamm.addToHistogram("Test-Set", "2");
  Pamm.addToHistogram("Test-Set", "2");
  Pamm.addToHistogram("Test-Set", "2");
  Pamm.addToHistogram("Test-Set", "2");
  Pamm.incCounter("setOpCount", 9);
  Pamm.stopTimer("timer3");
  Pamm.exportMeasuredData("HandleJSONOutputTest");

  EXPECT_EQ(30, Pamm.getCounter("setOpCount"));
  EXPECT_EQ(3, Pamm.getCounter("timerCount"));

  auto Timer1Elapsed = Pamm.elapsedTime("timer1");
  auto Timer2Elapsed = Pamm.elapsedTime("timer2");
  auto Timer3Elapsed = Pamm.elapsedTime("timer3");

  EXPECT_GE(Timer1Elapsed, 410); // 180+230
  EXPECT_LT(Timer1Elapsed, 820); // 180+230

  EXPECT_GE(Timer2Elapsed, 180);
  EXPECT_LT(Timer2Elapsed, 360);

  EXPECT_GE(Timer3Elapsed, 230);
  EXPECT_LT(Timer3Elapsed, 460);

  EXPECT_EQ(1, Pamm.getHistogram().size());
  EXPECT_EQ("Test-Set", Pamm.getHistogram().begin()->first());

  llvm::StringMap<uint64_t> Gt = {
      {"1", 10}, {"2", 4}, {"13", 3}, {"42", 13}, {"54", 3},
  };
  const auto &Hist = Pamm.getHistogram().begin()->second;
  EXPECT_EQ(Hist, Gt);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
