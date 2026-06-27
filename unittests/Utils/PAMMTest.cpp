#include "phasar/Utils/PAMM.h"

#include "phasar/Config/Configuration.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <thread>

namespace {
using namespace psr;
using namespace std::chrono_literals;

static constexpr pamm::Category<true> TestingCat("PAMMTest");

/* Test fixture */
class PAMMTest : public ::testing::Test {
protected:
  using time_point = std::chrono::high_resolution_clock::time_point;
  PAMMTest() = default;
  ~PAMMTest() override = default;

  void SetUp() override {}

  void TearDown() override { pamm::Registry::instance().clear(); }
};

TEST_F(PAMMTest, HandleTimer) {

  pamm::Timer<true, "timer1", &TestingCat> Tm1;

  {
    pamm::ScopedTimer TmScope{Tm1};
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
  }
  auto Elapsed = Tm1.elapsedNanos();
  EXPECT_GE(Elapsed, 120ms) << "Bad time measurement";
  EXPECT_LT(Elapsed, 240ms) << "Too much tolerance";

  pamm::printMeasuredData(llvm::outs());
}

TEST_F(PAMMTest, HandleCounter) {
  pamm::Counter<true, "first", &TestingCat> First;
  pamm::Counter<true, "second", &TestingCat> Second;
  pamm::Counter<true, "third", &TestingCat> Third;

  First += 42;
  Second++;
  ++Second;

  Third += 2;
  Third -= 2;

  EXPECT_EQ(First.value(), 42);
  EXPECT_EQ(Second.value(), 2);
  EXPECT_EQ(Third.value(), 0);

  pamm::printMeasuredData(llvm::outs());
}

TEST_F(PAMMTest, HandleHistogram) {
  pamm::Histogram<true, "Test-Set", &TestingCat> TestSet;
  TestSet.add(13, 1);
  TestSet.add(13, 1);
  TestSet.add(13, 1);

  TestSet.add(42, 1);
  TestSet.add(42, 1);
  TestSet.add(42, 1);
  TestSet.add(42, 1);
  TestSet.add(42, 1);

  TestSet.add(54, 1);
  TestSet.add(54, 1);
  TestSet.add(54, 1);

  TestSet.add(42, 1);

  TestSet.add(42, 7);

  TestSet.add(1, 4);
  TestSet.add(1, 1);
  TestSet.add(1, 1);
  TestSet.add(1, 1);
  TestSet.add(1, 1);
  TestSet.add(1, 1);
  TestSet.add(1, 1);

  TestSet.add(2, 4);

  llvm::StringMap<uint64_t> Gt = {
      {"1", 10}, {"2", 4}, {"13", 3}, {"42", 13}, {"54", 3},
  };
  const auto &Hist = TestSet.internalRawData();
  EXPECT_EQ(Hist, Gt);

  pamm::printMeasuredData(llvm::outs());
}

// TEST_F(PAMMTest, HandleJSONOutput) {
//   PAMM &Pamm = PAMM::getInstance();
//   Pamm.regCounter("timerCount");
//   Pamm.regCounter("setOpCount");
//   Pamm.startTimer("timer1");
//   Pamm.incCounter("timerCount");
//   Pamm.startTimer("timer2");
//   Pamm.incCounter("timerCount");
//   Pamm.regHistogram("Test-Set");

//   Pamm.incCounter("setOpCount", 11);

//   std::this_thread::sleep_for(std::chrono::milliseconds(180));
//   Pamm.stopTimer("timer2");

//   Pamm.incCounter("setOpCount", 10);

//   Pamm.startTimer("timer3");
//   Pamm.incCounter("timerCount");
//   std::this_thread::sleep_for(std::chrono::milliseconds(230));

//   Pamm.incCounter("setOpCount", 9);
//   Pamm.stopTimer("timer3");
//   Pamm.exportMeasuredData("HandleJSONOutputTest");

//   EXPECT_EQ(30, Pamm.getCounter("setOpCount"));
//   EXPECT_EQ(3, Pamm.getCounter("timerCount"));

//   auto Timer1Elapsed = Pamm.elapsedTime("timer1");
//   auto Timer2Elapsed = Pamm.elapsedTime("timer2");
//   auto Timer3Elapsed = Pamm.elapsedTime("timer3");

//   EXPECT_GE(Timer1Elapsed, 410); // 180+230
//   EXPECT_LT(Timer1Elapsed, 820); // 180+230

//   EXPECT_GE(Timer2Elapsed, 180);
//   EXPECT_LT(Timer2Elapsed, 360);

//   EXPECT_GE(Timer3Elapsed, 230);
//   EXPECT_LT(Timer3Elapsed, 460);

//   EXPECT_EQ(1, Pamm.getHistogram().size());
//   EXPECT_EQ("Test-Set", Pamm.getHistogram().begin()->first());

//   llvm::StringMap<uint64_t> Gt = {
//       {"1", 10}, {"2", 4}, {"13", 3}, {"42", 13}, {"54", 3},
//   };
//   const auto &Hist = Pamm.getHistogram().begin()->second;
//   EXPECT_EQ(Hist, Gt);
// }
} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
