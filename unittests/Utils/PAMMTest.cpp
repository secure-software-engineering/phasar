#include "phasar/Utils/PAMM.h"
#include "gtest/gtest.h"
#include <iostream>
#include <thread>

using namespace psr;

/* Test fixture */
class PAMMTest : public ::testing::Test {
protected:
  typedef std::chrono::high_resolution_clock::time_point time_point;
  PAMMTest() {}
  virtual ~PAMMTest() {}

  virtual void SetUp() {}

  virtual void TearDown() {
    PAMM &pamm = PAMM::getInstance();
    pamm.printMeasuredData(std::cout);
    pamm.reset();
  }
};

TEST_F(PAMMTest, HandleTimer) {
  //   PAMM &pamm = PAMM::getInstance();
  PAMM::getInstance().startTimer("timer1");
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  PAMM::getInstance().stopTimer("timer1");
  EXPECT_GE(PAMM::getInstance().elapsedTime("timer1"), 1200);
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

TEST_F(PAMMTest, HandleJSONOutput) {
  PAMM &pamm = PAMM::getInstance();
  pamm.regCounter("timerCount");
  pamm.regCounter("setOpCount");
  pamm.startTimer("timer1");
  pamm.incCounter("timerCount");
  pamm.startTimer("timer2");
  pamm.incCounter("timerCount");
  pamm.regHistogram("Test-Set");
  pamm.addToHistogram("Test-Set", "13");
  pamm.addToHistogram("Test-Set", "13");
  pamm.addToHistogram("Test-Set", "13");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "54");
  pamm.addToHistogram("Test-Set", "54");
  pamm.addToHistogram("Test-Set", "54");
  pamm.addToHistogram("Test-Set", "42");
  pamm.incCounter("setOpCount", 11);

  std::this_thread::sleep_for(std::chrono::milliseconds(1800));
  pamm.stopTimer("timer2");

  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "42");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.incCounter("setOpCount", 10);

  pamm.startTimer("timer3");
  pamm.incCounter("timerCount");
  std::this_thread::sleep_for(std::chrono::milliseconds(2300));

  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "1");
  pamm.addToHistogram("Test-Set", "2");
  pamm.addToHistogram("Test-Set", "2");
  pamm.addToHistogram("Test-Set", "2");
  pamm.addToHistogram("Test-Set", "2");
  pamm.incCounter("setOpCount", 9);
  pamm.stopTimer("timer3");
  pamm.exportMeasuredData("HandleJSONOutputTest");
}

TEST_F(PAMMTest, DISABLED_PerformanceTimerBasic) {
  time_point start_1 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  time_point end_1 = std::chrono::high_resolution_clock::now();
  time_point start_2 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  time_point end_2 = std::chrono::high_resolution_clock::now();
  time_point start_3 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  time_point end_3 = std::chrono::high_resolution_clock::now();
  time_point start_4 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  time_point end_4 = std::chrono::high_resolution_clock::now();
  time_point start_5 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  time_point end_5 = std::chrono::high_resolution_clock::now();
  time_point start_6 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  time_point end_6 = std::chrono::high_resolution_clock::now();
  time_point start_7 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  time_point end_7 = std::chrono::high_resolution_clock::now();
  time_point start_8 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  time_point end_8 = std::chrono::high_resolution_clock::now();
  time_point start_9 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  time_point end_9 = std::chrono::high_resolution_clock::now();
  time_point start_10 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  time_point end_10 = std::chrono::high_resolution_clock::now();
  time_point start_11 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  time_point end_11 = std::chrono::high_resolution_clock::now();
  time_point start_12 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  time_point end_12 = std::chrono::high_resolution_clock::now();
  time_point start_13 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  time_point end_13 = std::chrono::high_resolution_clock::now();
  time_point start_14 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  time_point end_14 = std::chrono::high_resolution_clock::now();
  time_point start_15 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20000));
  time_point end_15 = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_1 - start_1);
  std::cout << "timer_1 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_2 - start_2);
  std::cout << "timer_2 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_3 - start_3);
  std::cout << "timer_3 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_4 - start_4);
  std::cout << "timer_4 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_5 - start_5);
  std::cout << "timer_5 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_6 - start_6);
  std::cout << "timer_6 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_7 - start_7);
  std::cout << "timer_7 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_8 - start_8);
  std::cout << "timer_8 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_9 - start_9);
  std::cout << "timer_9 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_10 - start_10);
  std::cout << "timer_10 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_11 - start_11);
  std::cout << "timer_11 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_12 - start_12);
  std::cout << "timer_12 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_13 - start_13);
  std::cout << "timer_13 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_14 - start_14);
  std::cout << "timer_14 : " << duration.count() << std::endl;
  duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_15 - start_15);
  std::cout << "timer_15 : " << duration.count() << std::endl;
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
