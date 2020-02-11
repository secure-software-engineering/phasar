#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <string>

using namespace psr;

static unsigned CurrMulTwoEF_Id = 0;
static unsigned CurrAddTwoEF_Id = 0;

struct MyEFC : EdgeFunctionComposer<int> {
  MyEFC(std::shared_ptr<EdgeFunction<int>> F,
        std::shared_ptr<EdgeFunction<int>> G)
      : EdgeFunctionComposer<int>(F, G){};
  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> otherFunction) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
};

struct MulTwoEF : EdgeFunction<int>, std::enable_shared_from_this<MulTwoEF> {
private:
  const unsigned MulTwoEF_Id;

public:
  MulTwoEF(unsigned id) : MulTwoEF_Id(id){};
  int computeTarget(int source) override { return source * 2; };
  std::shared_ptr<EdgeFunction<int>>
  composeWith(std::shared_ptr<EdgeFunction<int>> secondFunction) override {
    return std::make_shared<MyEFC>(this->shared_from_this(), secondFunction);
  }
  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> otherFunction) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
  bool equal_to(std::shared_ptr<EdgeFunction<int>> other) const override {
    return this == other.get();
  }
  void print(std::ostream &os, bool isForDebug = false) const override {
    os << "MulTwoEF_" << MulTwoEF_Id;
  }
};

struct AddTwoEF : EdgeFunction<int>, std::enable_shared_from_this<AddTwoEF> {
private:
  const unsigned AddTwoEF_Id;

public:
  AddTwoEF(unsigned id) : AddTwoEF_Id(id){};
  int computeTarget(int source) override { return source + 2; };
  std::shared_ptr<EdgeFunction<int>>
  composeWith(std::shared_ptr<EdgeFunction<int>> secondFunction) override {
    return std::make_shared<MyEFC>(this->shared_from_this(), secondFunction);
  }
  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> otherFunction) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
  bool equal_to(std::shared_ptr<EdgeFunction<int>> other) const override {
    return this == other.get();
  }
  void print(std::ostream &os, bool isForDebug = false) const override {
    os << "AddTwoEF_" << AddTwoEF_Id;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  auto EF1 = std::make_shared<AddTwoEF>(++CurrAddTwoEF_Id);
  auto EF2 = std::make_shared<AddTwoEF>(++CurrAddTwoEF_Id);
  std::cout << "My EF : " << EF1->str() << " " << EF2->str() << '\n';
  EXPECT_EQ("AddTwoEF_1", EF1->str());
  EXPECT_EQ("AddTwoEF_2", EF2->str());
  auto EFC1 = std::make_shared<MyEFC>(EF1, EF2);
  auto EFC2 = std::make_shared<MyEFC>(EF2, EdgeIdentity<int>::getInstance());
  std::cout << "My EFC: " << EFC1->str() << " " << EFC2->str() << '\n';
  EXPECT_EQ("COMP[ AddTwoEF_1 , AddTwoEF_2 ] (EF:1)", EFC1->str());
  EXPECT_EQ("COMP[ AddTwoEF_2 , EdgeIdentity ] (EF:2)", EFC2->str());
  // Reset ID's for next test
  CurrAddTwoEF_Id = 0;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  int initialValue = 3;
  auto AddEF1 = std::make_shared<AddTwoEF>(++CurrAddTwoEF_Id);
  auto AddEF2 = std::make_shared<AddTwoEF>(++CurrAddTwoEF_Id);
  auto MulEF = std::make_shared<MulTwoEF>(++CurrMulTwoEF_Id);
  auto ComposedEF = (AddEF1->composeWith(MulEF))->composeWith(AddEF2);
  std::cout << "Compose: " << ComposedEF->str() << '\n';
  int result = ComposedEF->computeTarget(initialValue);
  std::cout << "Result: " << result << '\n';
  EXPECT_EQ(12, result);
  EXPECT_FALSE(AddEF1->equal_to(AddEF2));
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}