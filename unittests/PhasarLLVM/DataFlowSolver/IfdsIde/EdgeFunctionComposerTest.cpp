#include <gtest/gtest.h>
#include <iostream>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <string>

using namespace psr;

static unsigned CurrMulTwoEF_Id = 0;
static unsigned CurrAddTwoEF_Id = 0;

struct MyEFC : EdgeFunctionComposer<int> {
  MyEFC(EdgeFunction<int> *F, EdgeFunction<int> *G)
      : EdgeFunctionComposer<int>(F, G){};

  ~MyEFC() override = default;

  EdgeFunction<int> *joinWith(EdgeFunction<int> *otherFunction) override {
    return new AllBottom<int>(-1);
  }
};

struct MulTwoEF : EdgeFunction<int> {
private:
  const unsigned MulTwoEF_Id;

public:
  MulTwoEF(unsigned id) : MulTwoEF_Id(id){};

  ~MulTwoEF() override = default;

  int computeTarget(int source) override { return source * 2; };

  EdgeFunction<int> *composeWith(EdgeFunction<int> *secondFunction) override {
    return new MyEFC(this, secondFunction);
  }

  EdgeFunction<int> *joinWith(EdgeFunction<int> *otherFunction) override {
    return new AllBottom<int>(-1);
  };

  bool equal_to(EdgeFunction<int> *other) const override {
    return this == other;
  }

  void print(std::ostream &os, bool isForDebug = false) const override {
    os << "MulTwoEF_" << MulTwoEF_Id;
  }
};

struct AddTwoEF : EdgeFunction<int> {
private:
  const unsigned AddTwoEF_Id;

public:
  AddTwoEF(unsigned id) : AddTwoEF_Id(id){};

  ~AddTwoEF() override = default;

  int computeTarget(int source) override { return source + 2; };
 
  EdgeFunction<int> *composeWith(EdgeFunction<int> *secondFunction) override {
    return new MyEFC(this, secondFunction);
  }
 
  EdgeFunction<int> *joinWith(EdgeFunction<int> *otherFunction) override {
    return new AllBottom<int>(-1);
  };
 
  bool equal_to(EdgeFunction<int> *other) const override {
    return this == other;
  }
 
  void print(std::ostream &os, bool isForDebug = false) const override {
    os << "AddTwoEF_" << AddTwoEF_Id;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  auto EF1 = new AddTwoEF(++CurrAddTwoEF_Id);
  auto EF2 = new AddTwoEF(++CurrAddTwoEF_Id);
  std::cout << "My EF : " << EF1->str() << " " << EF2->str() << '\n';
  EXPECT_EQ("AddTwoEF_1", EF1->str());
  EXPECT_EQ("AddTwoEF_2", EF2->str());
  auto EFC1 = new MyEFC(EF1, EF2);
  auto EFC2 = new MyEFC(EF2, EdgeIdentity<int>::getInstance());
  std::cout << "My EFC: " << EFC1->str() << " " << EFC2->str() << '\n';
  EXPECT_EQ("COMP[ AddTwoEF_1 , AddTwoEF_2 ] (EF:1)", EFC1->str());
  EXPECT_EQ("COMP[ AddTwoEF_2 , EdgeId ] (EF:2)", EFC2->str());
  // Reset ID's for next test
  CurrAddTwoEF_Id = 0;
  delete EF1;
  delete EF2;
  delete EFC1;
  delete EFC2;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  int initialValue = 3;
  auto AddEF1 = new AddTwoEF(++CurrAddTwoEF_Id);
  auto AddEF2 = new AddTwoEF(++CurrAddTwoEF_Id);
  auto MulEF = new MulTwoEF(++CurrMulTwoEF_Id);
  auto ComposedEFIntermediate = AddEF1->composeWith(MulEF);
  auto ComposedEF = ComposedEFIntermediate->composeWith(AddEF2);
  std::cout << "Compose: " << ComposedEF->str() << '\n';
  int result = ComposedEF->computeTarget(initialValue);
  std::cout << "Result: " << result << '\n';
  EXPECT_EQ(12, result);
  EXPECT_FALSE(AddEF1->equal_to(AddEF2));
  delete AddEF1;
  delete AddEF2;
  delete MulEF;
  delete ComposedEFIntermediate;
  delete ComposedEF;
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}