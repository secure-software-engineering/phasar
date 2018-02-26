#include "../../src/utils/PAMM.h"
#include <gtest/gtest.h>
#include <thread>

/* Test fixture */
class PAMMTest : public ::testing::Test {
protected:
  PAMMTest() {}
  virtual ~PAMMTest() {}

  virtual void SetUp() {}

  virtual void TearDown() {
    PAMM &pamm = PAMM::getInstance();
    pamm.printData();
    pamm.reset();
  }
};

TEST_F(PAMMTest, HandleTimer) {
  PAMM &pamm = PAMM::getInstance();
  pamm.startTimer("timer1");
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  pamm.stopTimer("timer1");
  EXPECT_GE(pamm.elapsedTime<std::chrono::milliseconds>("timer1"), 1200);
  EXPECT_GE(pamm.getPrintableDuration(pamm.elapsedTime<std::chrono::milliseconds>("timer1")), "1sec 200ms");
}

TEST_F(PAMMTest, HandleCounter) {
  PAMM &pamm = PAMM::getInstance();
  pamm.regCounter("first");
  pamm.regCounter("second");
  pamm.regCounter("third");
  pamm.incCounter("first", 42);
  pamm.incCounter("second");
  pamm.incCounter("second");
  pamm.incCounter("third", 2);
  pamm.decCounter("third", 2);
  EXPECT_EQ(pamm.getCounter("first"), 42);
  EXPECT_EQ(pamm.getCounter("second"), 2);
  EXPECT_EQ(pamm.getCounter("third"), 0);
}

TEST_F(PAMMTest, HandleSetHisto) {
  PAMM &pamm = PAMM::getInstance();
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(2);
  pamm.addDataToSetHistogram(2);
  pamm.addDataToSetHistogram(2);
  EXPECT_EQ(pamm.getSetHistoData(13), 3);
  EXPECT_EQ(pamm.getSetHistoData(42), 13);
  EXPECT_EQ(pamm.getSetHistoData(1), 5);
  EXPECT_EQ(pamm.getSetHistoData(2), 3);
}

TEST_F(PAMMTest, HandleJSONOutput) {
  PAMM &pamm = PAMM::getInstance();
  pamm.regCounter("timerCount");
  pamm.regCounter("setOpCount");
  pamm.startTimer("timer1");
  pamm.incCounter("timerCount");
  pamm.startTimer("timer2");
  pamm.incCounter("timerCount");
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(13);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(54);
  pamm.addDataToSetHistogram(54);
  pamm.addDataToSetHistogram(54);
  pamm.addDataToSetHistogram(42);
  pamm.incCounter("setOpCount", 11);

  std::this_thread::sleep_for(std::chrono::milliseconds(1800));
  pamm.stopTimer("timer2");

  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(42);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.incCounter("setOpCount", 10);

  pamm.startTimer("timer3");
  pamm.incCounter("timerCount");
  std::this_thread::sleep_for(std::chrono::milliseconds(2300));

  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(1);
  pamm.addDataToSetHistogram(2);
  pamm.addDataToSetHistogram(2);
  pamm.addDataToSetHistogram(2);
  pamm.addDataToSetHistogram(2);
  pamm.incCounter("setOpCount", 9);
  pamm.stopTimer("timer3");
  pamm.exportDataAsJSON("HandleJSONOutputTest");
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
