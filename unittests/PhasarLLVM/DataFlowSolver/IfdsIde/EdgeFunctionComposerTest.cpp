#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace psr;

static unsigned CurrMulTwoEfId = 0;
static unsigned CurrAddTwoEfId = 0;

struct MyEFC : EdgeFunctionComposer<int> {
  MyEFC(EdgeFunctionPtrType F,
        EdgeFunctionPtrType G)
      : EdgeFunctionComposer<int>(std::move(F), std::move(G)){};
  EdgeFunctionPtrType
  joinWith(EdgeFunctionPtrType OtherFunction) override {
    return new AllBottom<int>(-1);
  };
};

struct MulTwoEF : EdgeFunction<int> {
private:
  const unsigned MulTwoEfId;

public:
  MulTwoEF(unsigned Id) : MulTwoEfId(Id){};
  int computeTarget(int Source) override { return Source * 2; };
  EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType SecondFunction) override {
    return new MyEFC(this, SecondFunction);
  }
  EdgeFunctionPtrType
  joinWith(EdgeFunctionPtrType OtherFunction) override {
    return new AllBottom<int>(-1);
  };
  bool equal_to(EdgeFunctionPtrType Other) const override {
    return this == Other;
  }
  void print(std::ostream &Os, bool IsForDebug = false) const override {
    Os << "MulTwoEF_" << MulTwoEfId;
  }
};

struct AddTwoEF : EdgeFunction<int> {
private:
  const unsigned AddTwoEfId;

public:
  AddTwoEF(unsigned Id) : AddTwoEfId(Id){};
  int computeTarget(int Source) override { return Source + 2; };
  EdgeFunctionPtrType
  composeWith(EdgeFunctionPtrType SecondFunction) override {
    return new MyEFC(this, SecondFunction);
  }
  EdgeFunctionPtrType
  joinWith(EdgeFunctionPtrType OtherFunction) override {
    return new AllBottom<int>(-1);
  };
  bool equal_to(EdgeFunctionPtrType Other) const override {
    return this == Other;
  }
  void print(std::ostream &Os, bool IsForDebug = false) const override {
    Os << "AddTwoEF_" << AddTwoEfId;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  auto EF1 = new AddTwoEF(++CurrAddTwoEfId);
  auto EF2 = new AddTwoEF(++CurrAddTwoEfId);
  std::cout << "My EF : " << EF1->str() << " " << EF2->str() << '\n';
  EXPECT_EQ("AddTwoEF_1", EF1->str());
  EXPECT_EQ("AddTwoEF_2", EF2->str());
  auto EFC1 = new MyEFC(EF1, EF2);
  auto EFC2 = new MyEFC(EF2, EdgeIdentity<int>::getInstance());
  std::cout << "My EFC: " << EFC1->str() << " " << EFC2->str() << '\n';
  EXPECT_EQ("COMP[ AddTwoEF_1 , AddTwoEF_2 ] (EF:1)", EFC1->str());
  EXPECT_EQ("COMP[ AddTwoEF_2 , EdgeIdentity ] (EF:2)", EFC2->str());
  // Reset ID's for next test
  CurrAddTwoEfId = 0;
  delete EF1;
  delete EF2;
  delete EFC1;
  delete EFC2;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  int InitialValue = 3;
  auto AddEF1 = new AddTwoEF(++CurrAddTwoEfId);
  auto AddEF2 = new AddTwoEF(++CurrAddTwoEfId);
  auto MulEF = new MulTwoEF(++CurrMulTwoEfId);
  auto ComposedEFIntermediate = AddEF1->composeWith(MulEF);
  auto ComposedEF = ComposedEFIntermediate->composeWith(AddEF2);
  std::cout << "Compose: " << ComposedEF->str() << '\n';
  int Result = ComposedEF->computeTarget(InitialValue);
  std::cout << "Result: " << Result << '\n';
  EXPECT_EQ(12, Result);
  EXPECT_FALSE(AddEF1->equal_to(AddEF2));
  delete AddEF1;
  delete AddEF2;
  delete MulEF;
  delete ComposedEFIntermediate;
  delete ComposedEF;
}


// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}