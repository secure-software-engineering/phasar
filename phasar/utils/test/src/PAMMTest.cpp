#include "phasar/Utils/PAMM.h"
#include "gtest/gtest.h"
#include <thread>

#include "llvm/Support/raw_ostream.h"

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
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  PAMM::getInstance().stopTimer("timer1");
  EXPECT_GE(PAMM::getInstance().elapsedTime("timer1"), 1200U);
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

  std::this_thread::sleep_for(std::chrono::milliseconds(1800));
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
  std::this_thread::sleep_for(std::chrono::milliseconds(2300));

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
}

TEST_F(PAMMTest, DISABLED_PerformanceTimerBasic) {
  time_point Start1 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  time_point End1 = std::chrono::high_resolution_clock::now();
  time_point Start2 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  time_point End2 = std::chrono::high_resolution_clock::now();
  time_point Start3 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  time_point End3 = std::chrono::high_resolution_clock::now();
  time_point Start4 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  time_point End4 = std::chrono::high_resolution_clock::now();
  time_point Start5 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  time_point End5 = std::chrono::high_resolution_clock::now();
  time_point Start6 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  time_point End6 = std::chrono::high_resolution_clock::now();
  time_point Start7 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  time_point End7 = std::chrono::high_resolution_clock::now();
  time_point Start8 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  time_point End8 = std::chrono::high_resolution_clock::now();
  time_point Start9 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  time_point End9 = std::chrono::high_resolution_clock::now();
  time_point Start10 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  time_point End10 = std::chrono::high_resolution_clock::now();
  time_point Start11 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  time_point End11 = std::chrono::high_resolution_clock::now();
  time_point Start12 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  time_point End12 = std::chrono::high_resolution_clock::now();
  time_point Start13 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  time_point End13 = std::chrono::high_resolution_clock::now();
  time_point Start14 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  time_point End14 = std::chrono::high_resolution_clock::now();
  time_point Start15 = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20000));
  time_point End15 = std::chrono::high_resolution_clock::now();
  auto Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End1 - Start1);
  llvm::outs() << "timer_1 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End2 - Start2);
  llvm::outs() << "timer_2 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End3 - Start3);
  llvm::outs() << "timer_3 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End4 - Start4);
  llvm::outs() << "timer_4 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End5 - Start5);
  llvm::outs() << "timer_5 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End6 - Start6);
  llvm::outs() << "timer_6 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End7 - Start7);
  llvm::outs() << "timer_7 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End8 - Start8);
  llvm::outs() << "timer_8 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End9 - Start9);
  llvm::outs() << "timer_9 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End10 - Start10);
  llvm::outs() << "timer_10 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End11 - Start11);
  llvm::outs() << "timer_11 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End12 - Start12);
  llvm::outs() << "timer_12 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End13 - Start13);
  llvm::outs() << "timer_13 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End14 - Start14);
  llvm::outs() << "timer_14 : " << Duration.count() << '\n';
  Duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(End15 - Start15);
  llvm::outs() << "timer_15 : " << Duration.count() << '\n';
}
