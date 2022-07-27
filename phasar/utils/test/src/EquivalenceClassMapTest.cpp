#include "gtest/gtest.h"
#include <optional>
#include <utility>
#include <vector>

#include "phasar/Utils/EquivalenceClassMap.h"

using namespace psr;
using namespace std;

TEST(EquivalenceClassMap, ctorEmpty) {
  EquivalenceClassMap<int, std::string> M;
  EXPECT_EQ(M.size(), 0U);
}

TEST(EquivalenceClassMap, ctorIter) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  std::vector<std::pair<MapTy::key_type, MapTy::mapped_type>> InitValues(
      {make_pair(42, "foo"), make_pair(21, "bar")});

  MapTy M{InitValues.begin(), InitValues.end()};

  EXPECT_EQ(M.size(), 2U);
  EXPECT_EQ(M.find(42)->first.count(42), 1U);
  EXPECT_EQ(M.find(42)->second, "foo");
  EXPECT_EQ(M.find(21)->first.count(21), 1U);
  EXPECT_EQ(M.find(21)->second, "bar");
}

TEST(EquivalenceClassMap, ctorInitList) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar")}};

  EXPECT_EQ(M.size(), 2U);
  EXPECT_EQ(M.find(42)->first.count(42), 1U);
  EXPECT_EQ(M.find(42)->second, "foo");
  EXPECT_EQ(M.find(21)->first.count(21), 1U);
  EXPECT_EQ(M.find(21)->second, "bar");
}

TEST(EquivalenceClassMap, iterMap) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar")}};

  auto Iter = M.begin();
  EXPECT_EQ(Iter->first.count(42), 1U);
  EXPECT_EQ(Iter->second, "foo");
  ++Iter;
  EXPECT_EQ(Iter->first.count(21), 1U);
  EXPECT_EQ(Iter->second, "bar");
  ++Iter;
  EXPECT_EQ(Iter, M.end());
}

TEST(EquivalenceClassMap, insertKeyRef) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;
  int Key = 42;
  const MapTy::key_type &KeyRef = Key;

  M.insert(42, "foo");
  EXPECT_EQ(M.findValue(42), "foo");
}

TEST(EquivalenceClassMap, insertKeyRVal) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;

  M.insert(42, "foo");
  EXPECT_EQ(M.findValue(42), "foo");
}

TEST(EquivalenceClassMap, insertKeyValuePairRef) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;
  const std::pair<int, std::string> KVPair{42, "foo"};

  M.insert(KVPair);
  EXPECT_EQ(M.findValue(42), "foo");
}

TEST(EquivalenceClassMap, insertKeyValueRVal) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;

  M.insert(std::make_pair(42, "foo"));
  EXPECT_EQ(M.findValue(42), "foo");
}

TEST(EquivalenceClassMap, count) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar"), make_pair(20, "foo")}};

  EXPECT_EQ(M.count(42), 1U);
  EXPECT_EQ(M.count(21), 1U);
  EXPECT_EQ(M.count(20), 1U);
  EXPECT_EQ(M.count(1337), 0U);
}

TEST(EquivalenceClassMap, numEqClasses) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar"), make_pair(20, "foo")}};

  EXPECT_EQ(M.numEquivalenceClasses(), 2U);
  EXPECT_EQ(M.size(), 2U);
}

TEST(EquivalenceClassMap, findEntryForKey) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar"), make_pair(20, "foo")}};

  EXPECT_EQ(M.find(42)->second, "foo");
  EXPECT_EQ(M.find(21)->second, "bar");
  EXPECT_EQ(M.find(20)->second, "foo");
}

TEST(EquivalenceClassMap, findValueForKey) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar"), make_pair(20, "foo")}};

  EXPECT_EQ(M.findValue(42), "foo");
  EXPECT_EQ(M.findValue(21), "bar");
  EXPECT_EQ(M.findValue(20), "foo");
  EXPECT_EQ(M.findValue(1337), std::nullopt);
}

TEST(EquivalenceClassMap, clear) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M{{make_pair(42, "foo"), make_pair(21, "bar"), make_pair(20, "foo")}};

  EXPECT_EQ(M.numEquivalenceClasses(), 2U);
  M.clear();
  EXPECT_EQ(M.numEquivalenceClasses(), 0U);
}

//===----------------------------------------------------------------------===//
// Equivalence functionality tests
TEST(EquivalenceClassMap, threeDifferentValues) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;

  M.insert(std::make_pair(42, "foo"));
  M.insert(std::make_pair(21, "bar"));
  M.insert(std::make_pair(40, "bazz"));

  EXPECT_EQ(M.findValue(42), "foo");
  EXPECT_EQ(M.findValue(21), "bar");
  EXPECT_EQ(M.findValue(40), "bazz");

  EXPECT_EQ(M.numEquivalenceClasses(), 3U);
}

TEST(EquivalenceClassMap, threeEqualValues) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;

  M.insert(std::make_pair(42, "foo"));
  M.insert(std::make_pair(21, "foo"));
  M.insert(std::make_pair(40, "foo"));

  EXPECT_EQ(M.findValue(42), "foo");
  EXPECT_EQ(M.findValue(21), "foo");
  EXPECT_EQ(M.findValue(40), "foo");

  EXPECT_EQ(M.numEquivalenceClasses(), 1U);
}

TEST(EquivalenceClassMap, threeMixedValues) {
  using MapTy = EquivalenceClassMap<int, std::string>;
  MapTy M;

  M.insert(std::make_pair(42, "foo"));
  M.insert(std::make_pair(21, "bar"));
  M.insert(std::make_pair(40, "foo"));

  EXPECT_EQ(M.findValue(42), "foo");
  EXPECT_EQ(M.findValue(21), "bar");
  EXPECT_EQ(M.findValue(40), "foo");

  EXPECT_EQ(M.numEquivalenceClasses(), 2U);
}
