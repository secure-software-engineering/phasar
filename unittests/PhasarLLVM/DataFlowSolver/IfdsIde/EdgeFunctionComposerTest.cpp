#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"

#include "gtest/gtest.h"

#include <memory>
#include <string>
#include <utility>

using namespace psr;

static unsigned CurrMulTwoEfId = 0;
static unsigned CurrAddTwoEfId = 0;

struct MyEFC : EdgeFunctionComposer<int> {
  MyEFC(std::shared_ptr<EdgeFunction<int>> F,
        std::shared_ptr<EdgeFunction<int>> G)
      : EdgeFunctionComposer<int>(F, G){};

  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> /*OtherFunction*/) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
};

struct MulTwoEF : EdgeFunction<int>, std::enable_shared_from_this<MulTwoEF> {
private:
  const unsigned MulTwoEfId;

public:
  MulTwoEF(unsigned Id) : MulTwoEfId(Id){};
  int computeTarget(int Source) override { return Source * 2; };
  std::shared_ptr<EdgeFunction<int>>
  composeWith(std::shared_ptr<EdgeFunction<int>> SecondFunction) override {
    return std::make_shared<MyEFC>(this->shared_from_this(), SecondFunction);
  }
  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> /*OtherFunction*/) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
  bool equal_to(std::shared_ptr<EdgeFunction<int>> Other) const override {
    return this == Other.get();
  }
  void print(llvm::raw_ostream &Os,
             bool /*IsForDebug = false*/) const override {
    Os << "MulTwoEF_" << MulTwoEfId;
  }
};

struct AddTwoEF : EdgeFunction<int>, std::enable_shared_from_this<AddTwoEF> {
private:
  const unsigned AddTwoEfId;

public:
  AddTwoEF(unsigned Id) : AddTwoEfId(Id){};
  int computeTarget(int Source) override { return Source + 2; };
  std::shared_ptr<EdgeFunction<int>>
  composeWith(std::shared_ptr<EdgeFunction<int>> SecondFunction) override {
    return std::make_shared<MyEFC>(this->shared_from_this(), SecondFunction);
  }
  std::shared_ptr<EdgeFunction<int>>
  joinWith(std::shared_ptr<EdgeFunction<int>> /*OtherFunction*/) override {
    return std::make_shared<AllBottom<int>>(-1);
  };
  bool equal_to(std::shared_ptr<EdgeFunction<int>> Other) const override {
    return this == Other.get();
  }
  void print(llvm::raw_ostream &Os,
             bool /*IsForDebug = false*/) const override {
    Os << "AddTwoEF_" << AddTwoEfId;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  auto EF1 = std::make_shared<AddTwoEF>(++CurrAddTwoEfId);
  auto EF2 = std::make_shared<AddTwoEF>(++CurrAddTwoEfId);
  llvm::outs() << "My EF : " << EF1->str() << " " << EF2->str() << '\n';
  EXPECT_EQ("AddTwoEF_1", EF1->str());
  EXPECT_EQ("AddTwoEF_2", EF2->str());
  auto EFC1 = std::make_shared<MyEFC>(EF1, EF2);
  auto EFC2 = std::make_shared<MyEFC>(EF2, EdgeIdentity<int>::getInstance());
  llvm::outs() << "My EFC: " << EFC1->str() << " " << EFC2->str() << '\n';
  EXPECT_EQ("COMP[ AddTwoEF_1 , AddTwoEF_2 ] (EF:1)", EFC1->str());
  EXPECT_EQ("COMP[ AddTwoEF_2 , EdgeIdentity ] (EF:2)", EFC2->str());
  // Reset ID's for next test
  CurrAddTwoEfId = 0;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  int InitialValue = 3;
  auto AddEF1 = std::make_shared<AddTwoEF>(++CurrAddTwoEfId);
  auto AddEF2 = std::make_shared<AddTwoEF>(++CurrAddTwoEfId);
  auto MulEF = std::make_shared<MulTwoEF>(++CurrMulTwoEfId);
  auto ComposedEF = (AddEF1->composeWith(MulEF))->composeWith(AddEF2);
  llvm::outs() << "Compose: " << ComposedEF->str() << '\n';
  int Result = ComposedEF->computeTarget(InitialValue);
  llvm::outs() << "Result: " << Result << '\n';
  EXPECT_EQ(12, Result);
  EXPECT_FALSE(AddEF1->equal_to(AddEF2));
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
