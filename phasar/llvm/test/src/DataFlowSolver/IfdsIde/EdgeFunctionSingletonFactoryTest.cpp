#include "gtest/gtest.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"

#include <chrono>
#include <thread>

namespace psr::internal {
struct TestEdgeFunction
    : public EdgeFunction<int>,
      public std::enable_shared_from_this<TestEdgeFunction>,
      public EdgeFunctionSingletonFactory<TestEdgeFunction, int> {

  TestEdgeFunction(int Val) : Val(Val) {}

  static decltype(auto) getTestCacheData() {
    return EdgeFunctionSingletonFactory<TestEdgeFunction, int>::getCacheData();
  }

  int computeTarget(int /*Source*/) override { return 42; }

  EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType /*SecondFunction*/) override {
    return this->shared_from_this();
  };

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType /*OtherFunction*/) override {
    return this->shared_from_this();
  }

  bool equal_to(EdgeFunctionPtrType /*OtherFunction*/) const override {
    return false;
  };

  int Val;
};
} // namespace psr::internal

using namespace psr;
using namespace psr::internal;

//===----------------------------------------------------------------------===//
// Direct api usage tests

TEST(EdgeFunctionSingletonFactoryTest, createEdgeFunctions) {
  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  auto EF2 = TestEdgeFunction::createEdgeFunction(1337);

  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 2U);
}

TEST(EdgeFunctionSingletonFactoryTest, createEdgeFunctionsWithCorrectData) {
  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  auto EF2 = TestEdgeFunction::createEdgeFunction(1337);

  EXPECT_EQ(EF1->Val, 42);
  EXPECT_EQ(EF2->Val, 1337);
}

TEST(EdgeFunctionSingletonFactoryTest, createEdgeFunctionsTwice) {
  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  auto EF2 = TestEdgeFunction::createEdgeFunction(1337);
  auto EF3 = TestEdgeFunction::createEdgeFunction(42);

  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 2U);
  EXPECT_EQ(EF1.get(), EF3.get());
}

TEST(EdgeFunctionSingletonFactoryTest, selfCleanExpiredEdgeFunctions) {
  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  {
    auto EF2 = TestEdgeFunction::createEdgeFunction(1337);
  } // EF2 deleted after scope

  TestEdgeFunction::cleanExpiredEdgeFunctions();

  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 1U);
}

//===----------------------------------------------------------------------===//
// Threaded tests

TEST(EdgeFunctionSingletonFactoryTest, createEdgeFunctionsThreaded) {
  TestEdgeFunction::initEdgeFunctionCleaner();

  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  auto EF2 = TestEdgeFunction::createEdgeFunction(1337);

  std::lock_guard<std::mutex> DataLock(
      TestEdgeFunction::getTestCacheData().DataMutex);
  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 2U);
}

TEST(EdgeFunctionSingletonFactoryTest, createEdgeFunctionsTwiceThreaded) {
  TestEdgeFunction::initEdgeFunctionCleaner();

  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  auto EF2 = TestEdgeFunction::createEdgeFunction(1337);
  auto EF3 = TestEdgeFunction::createEdgeFunction(42);

  std::lock_guard<std::mutex> DataLock(
      TestEdgeFunction::getTestCacheData().DataMutex);
  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 2U);
  EXPECT_EQ(EF1.get(), EF3.get());
}

TEST(EdgeFunctionSingletonFactoryTest, selfCleanExpiredEdgeFunctionsThreaded) {
  TestEdgeFunction::initEdgeFunctionCleaner();

  auto EF1 = TestEdgeFunction::createEdgeFunction(42);
  {
    auto EF2 = TestEdgeFunction::createEdgeFunction(1337);
  } // EF2 deleted after scope

  std::this_thread::sleep_for(std::chrono::seconds{3});

  std::lock_guard<std::mutex> DataLock(
      TestEdgeFunction::getTestCacheData().DataMutex);
  EXPECT_EQ(TestEdgeFunction::getTestCacheData().Storage.size(), 1U);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
