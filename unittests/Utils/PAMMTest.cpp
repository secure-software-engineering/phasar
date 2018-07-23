#include <gtest/gtest.h>

#include <thread>

#include <phasar/Utils/PAMM.h>

#ifdef PERFORMANCE_EVA // We need it to enable PAMM whatever the build configuration ;)

using namespace std;
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
  EXPECT_GE(pamm.getPrintableDuration(
                pamm.elapsedTime<std::chrono::milliseconds>("timer1")),
            "1sec 200ms");
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

// TEST_F(PAMMTest, HandleSetHisto) {
//   PAMM &pamm = PAMM::getInstance();
//   pamm.regHistogram("Test-Set");
//   pamm.addToHistogram("Test-Set", "13");
//   pamm.addToHistogram("Test-Set", "13");
//   pamm.addToHistogram("Test-Set", "13");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "42");
//   pamm.addToHistogram("Test-Set", "1");
//   pamm.addToHistogram("Test-Set", "1");
//   pamm.addToHistogram("Test-Set", "1");
//   pamm.addToHistogram("Test-Set", "1");
//   pamm.addToHistogram("Test-Set", "1");
//   pamm.addToHistogram("Test-Set", "2");
//   pamm.addToHistogram("Test-Set", "2");
//   pamm.addToHistogram("Test-Set", "2");
//   EXPECT_EQ(pamm.getHistoData("Test-Set", "13"), 3);
//   EXPECT_EQ(pamm.getHistoData("Test-Set", "42"), 13);
//   EXPECT_EQ(pamm.getHistoData("Test-Set", "1"), 5);
//   EXPECT_EQ(pamm.getHistoData("Test-Set", "2"), 3);
// }

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
  pamm.exportDataAsJSON("HandleJSONOutputTest");
}

TEST_F(PAMMTest, PerformanceTimerBasic) {
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

// TEST_F(PAMMTest, PerformanceTimerPAMM) {
//   PAMM &pamm = PAMM::getInstance();
//   pamm.startTimer("timer_1");
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));
//   pamm.stopTimer("timer_1");
//   pamm.startTimer("timer_2");
//   std::this_thread::sleep_for(std::chrono::milliseconds(20));
//   pamm.stopTimer("timer_2");
//   pamm.startTimer("timer_3");
//   std::this_thread::sleep_for(std::chrono::milliseconds(30));
//   pamm.stopTimer("timer_3");
//   pamm.startTimer("timer_4");
//   std::this_thread::sleep_for(std::chrono::milliseconds(40));
//   pamm.stopTimer("timer_4");
//   pamm.startTimer("timer_5");
//   std::this_thread::sleep_for(std::chrono::milliseconds(50));
//   pamm.stopTimer("timer_5");
//   pamm.startTimer("timer_6");
//   std::this_thread::sleep_for(std::chrono::milliseconds(100));
//   pamm.stopTimer("timer_6");
//   pamm.startTimer("timer_7");
//   std::this_thread::sleep_for(std::chrono::milliseconds(150));
//   pamm.stopTimer("timer_7");
//   pamm.startTimer("timer_8");
//   std::this_thread::sleep_for(std::chrono::milliseconds(200));
//   pamm.stopTimer("timer_8");
//   pamm.startTimer("timer_9");
//   std::this_thread::sleep_for(std::chrono::milliseconds(300));
//   pamm.stopTimer("timer_9");
//   pamm.startTimer("timer_10");
//   std::this_thread::sleep_for(std::chrono::milliseconds(500));
//   pamm.stopTimer("timer_10");
//   pamm.startTimer("timer_11");
//   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//   pamm.stopTimer("timer_11");
//   pamm.startTimer("timer_12");
//   std::this_thread::sleep_for(std::chrono::milliseconds(2000));
//   pamm.stopTimer("timer_12");
//   pamm.startTimer("timer_13");
//   std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//   pamm.stopTimer("timer_13");
//   pamm.startTimer("timer_14");
//   std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//   pamm.stopTimer("timer_14");
//   pamm.startTimer("timer_15");
//   std::this_thread::sleep_for(std::chrono::milliseconds(20000));
//   pamm.stopTimer("timer_15");
//   pamm.printStoppedTimer<std::chrono::milliseconds>();
// }

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif
