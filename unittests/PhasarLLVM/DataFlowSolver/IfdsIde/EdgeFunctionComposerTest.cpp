#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarClang/MyMatcher.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
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
  MyEFC(EdgeFunctionPtrType F, EdgeFunctionPtrType G)
      : EdgeFunctionComposer<int>(F, G){};
  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction,
                               EFMemoryManager &MemoryManager) override {
    return MemoryManager.make_edge_function<AllBottom<int>>(-1);
  };
};

struct MulTwoEF : EdgeFunction<int> {
private:
  const unsigned MulTwoEfId;

public:
  MulTwoEF(unsigned Id) : MulTwoEfId(Id){};
  int computeTarget(int Source) override { return Source * 2; };
  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction,
                                  EFMemoryManager &MemoryManager) override {
    return MemoryManager.make_edge_function<MyEFC>(this, SecondFunction);
  }
  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction,
                               EFMemoryManager &MemoryManager) override {
    return MemoryManager.make_edge_function<AllBottom<int>>(-1);
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
  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction,
                                  EFMemoryManager &MemoryManager) override {
    return MemoryManager.make_edge_function<MyEFC>(this, SecondFunction);
  }
  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction,
                               EFMemoryManager &MemoryManager) override {
    return MemoryManager.make_edge_function<AllBottom<int>>(-1);
  };
  bool equal_to(EdgeFunctionPtrType Other) const override {
    return this == Other;
  }
  void print(std::ostream &Os, bool IsForDebug = false) const override {
    Os << "AddTwoEF_" << AddTwoEfId;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  EdgeFunctionMemoryManager<AddTwoEF::EdgeFunctionPtrType> MemManager;
  auto *EF1 = MemManager.make_edge_function<AddTwoEF>(++CurrAddTwoEfId);
  auto *EF2 = MemManager.make_edge_function<AddTwoEF>(++CurrAddTwoEfId);
  std::cout << "My EF : " << EF1->str() << " " << EF2->str() << '\n';
  EXPECT_EQ("AddTwoEF_1", EF1->str());
  EXPECT_EQ("AddTwoEF_2", EF2->str());
  auto *EFC1 = MemManager.make_edge_function<MyEFC>(EF1, EF2);
  auto *EFC2 = MemManager.make_edge_function<MyEFC>(
      EF2, EdgeIdentity<int>::getInstance());
  std::cout << "My EFC: " << EFC1->str() << " " << EFC2->str() << '\n';
  EXPECT_EQ("COMP[ AddTwoEF_1 , AddTwoEF_2 ] (EF:1)", EFC1->str());
  EXPECT_EQ("COMP[ AddTwoEF_2 , EdgeIdentity ] (EF:2)", EFC2->str());
  // Reset ID's for next test
  CurrAddTwoEfId = 0;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  EdgeFunctionMemoryManager<AddTwoEF::EdgeFunctionPtrType> MemManager;
  int InitialValue = 3;
  auto *AddEF1 = MemManager.make_edge_function<AddTwoEF>(++CurrAddTwoEfId);
  auto *AddEF2 = MemManager.make_edge_function<AddTwoEF>(++CurrAddTwoEfId);
  auto *MulEF = MemManager.make_edge_function<MulTwoEF>(++CurrMulTwoEfId);
  auto *ComposedEF =
      (AddEF1->composeWith(MulEF, MemManager))->composeWith(AddEF2, MemManager);
  std::cout << "Compose: " << ComposedEF->str() << '\n';
  int Result = ComposedEF->computeTarget(InitialValue);
  std::cout << "Result: " << Result << '\n';
  EXPECT_EQ(12, Result);
  EXPECT_FALSE(AddEF1->equal_to(AddEF2));
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
