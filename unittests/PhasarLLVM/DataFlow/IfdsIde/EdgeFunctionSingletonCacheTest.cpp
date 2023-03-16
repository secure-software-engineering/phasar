#include "phasar/DataFlow/IfdsIde/DefaultEdgeFunctionSingletonCache.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

namespace psr::internal {
struct TestEdgeFunction {
  using l_t = int;

  [[nodiscard]] int computeTarget(int /*Source*/) const { return Val; }

  static EdgeFunction<int>
  compose(EdgeFunctionRef<TestEdgeFunction> This,
          const EdgeFunction<int> & /*SecondFunction*/) {
    return This;
  };

  static EdgeFunction<int> join(EdgeFunctionRef<TestEdgeFunction> This,
                                const EdgeFunction<int> & /*OtherFunction*/) {
    return This;
  }

  bool operator==(const TestEdgeFunction &Other) const noexcept {
    return Val == Other.Val;
  }

  TestEdgeFunction(int Val) noexcept : Val(Val) {}
  TestEdgeFunction(int Val, bool *Deleted) noexcept
      : Val(Val), Deleted(Deleted) {}
  TestEdgeFunction(const TestEdgeFunction &Other) noexcept : Val(Other.Val) {
    // Non-trivial copy ctor to prevent SOO, such that we can actually see
    // caching
  }
  ~TestEdgeFunction() {
    if (Deleted) {
      *Deleted = true;
    }
  }

  friend llvm::hash_code hash_value(const TestEdgeFunction &EF) noexcept {
    return llvm::hash_value(EF.Val);
  }

  int Val;
  bool *Deleted{};
};
} // namespace psr::internal

using namespace psr;
using namespace psr::internal;

//===----------------------------------------------------------------------===//
// Direct api usage tests

TEST(EdgeFunctionSingletonCacheTest, createEdgeFunctions) {
  DefaultEdgeFunctionSingletonCache<TestEdgeFunction> Cache;

  EdgeFunction<int> EF1 = CachedEdgeFunction{TestEdgeFunction{42}, &Cache};
  EdgeFunction<int> EF2 = CachedEdgeFunction{TestEdgeFunction{1337}, &Cache};

  EXPECT_NE(EF1.getOpaqueValue(), EF2.getOpaqueValue());
}

TEST(EdgeFunctionSingletonCacheTest, createEdgeFunctionsWithCorrectData) {
  DefaultEdgeFunctionSingletonCache<TestEdgeFunction> Cache;

  EdgeFunction<int> EF1 = CachedEdgeFunction{TestEdgeFunction(42), &Cache};
  EdgeFunction<int> EF2 = CachedEdgeFunction{TestEdgeFunction(1337), &Cache};

  EXPECT_EQ(EF1.computeTarget(0), 42);
  EXPECT_EQ(EF2.computeTarget(0), 1337);
}

TEST(EdgeFunctionSingletonCacheTest, createEdgeFunctionsTwice) {
  DefaultEdgeFunctionSingletonCache<TestEdgeFunction> Cache;

  EdgeFunction<int> EF1 = CachedEdgeFunction{TestEdgeFunction(42), &Cache};
  EdgeFunction<int> EF2 = CachedEdgeFunction{TestEdgeFunction(1337), &Cache};
  EdgeFunction<int> EF3 = CachedEdgeFunction{TestEdgeFunction(42), &Cache};

  EXPECT_NE(EF1.getOpaqueValue(), EF2.getOpaqueValue());
  EXPECT_EQ(EF1.getOpaqueValue(), EF3.getOpaqueValue());
}

TEST(EdgeFunctionSingletonCacheTest, selfCleanExpiredEdgeFunctions) {
  DefaultEdgeFunctionSingletonCache<TestEdgeFunction> Cache;
  EdgeFunction<int> EF1 = CachedEdgeFunction{TestEdgeFunction(42), &Cache};
  bool Deleted = false;
  {
    EdgeFunction<int> EF2 =
        CachedEdgeFunction{TestEdgeFunction(1337, &Deleted), &Cache};
  } // EF2 deleted after scope

  EXPECT_TRUE(Deleted);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
