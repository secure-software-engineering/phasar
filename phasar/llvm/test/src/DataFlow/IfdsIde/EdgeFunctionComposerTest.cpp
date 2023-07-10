#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"

#include "gtest/gtest.h"

#include <memory>
#include <string>
#include <utility>

using namespace psr;

static unsigned CurrMulTwoEfId = 0;
static unsigned CurrAddTwoEfId = 0;

struct MyEFC : EdgeFunctionComposer<int> {
  static EdgeFunction<int> join(EdgeFunctionRef<MyEFC> /*This*/,
                                const EdgeFunction<int> & /*OtherFunction*/) {
    return AllBottom<int>{-1};
  };
};

struct MulTwoEF {
  using l_t = int;
  unsigned MulTwoEfId;

  [[nodiscard]] int computeTarget(int Source) const { return Source * 2; };
  static EdgeFunction<int> compose(EdgeFunctionRef<MulTwoEF> This,
                                   const EdgeFunction<int> &SecondFunction) {
    return MyEFC{This, SecondFunction};
  }
  static EdgeFunction<int> join(EdgeFunctionRef<MulTwoEF> /*This*/,
                                const EdgeFunction<int> & /*OtherFunction*/) {
    return AllBottom<int>{-1};
  };
  bool operator==(MulTwoEF /*Other*/) const noexcept { return true; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Os, MulTwoEF EF) {
    return Os << "MulTwoEF_" << EF.MulTwoEfId;
  }
};

struct AddTwoEF {
  using l_t = int;
  unsigned AddTwoEfId;

  [[nodiscard]] int computeTarget(int Source) const { return Source + 2; };

  static EdgeFunction<int> compose(EdgeFunctionRef<AddTwoEF> This,
                                   const EdgeFunction<int> &SecondFunction) {
    return MyEFC{This, SecondFunction};
  }
  static EdgeFunction<int> join(EdgeFunctionRef<AddTwoEF> /*This*/,
                                const EdgeFunction<int> & /*OtherFunction*/) {
    return AllBottom<int>{-1};
  };
  bool operator==(AddTwoEF /*Other*/) const noexcept { return true; }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Os, AddTwoEF EF) {
    return Os << "AddTwoEF_" << EF.AddTwoEfId;
  }
};

TEST(EdgeFunctionComposerTest, HandleEFIDs) {
  EdgeFunction<int> EF1 = AddTwoEF{++CurrAddTwoEfId};
  EdgeFunction<int> EF2 = AddTwoEF{++CurrAddTwoEfId};
  llvm::outs() << "My EF : " << EF1 << " " << EF2 << '\n';
  EXPECT_EQ("AddTwoEF_1", to_string(EF1));
  EXPECT_EQ("AddTwoEF_2", to_string(EF2));
  EdgeFunction<int> EFC1 = MyEFC{EF1, EF2};
  EdgeFunction<int> EFC2 = MyEFC{EF2, EdgeIdentity<int>{}};
  llvm::outs() << "My EFC: " << EFC1 << " " << EFC2 << '\n';
  EXPECT_EQ("EFComposer[AddTwoEF_1, AddTwoEF_2]", to_string(EFC1));
  EXPECT_EQ("EFComposer[AddTwoEF_2, psr::EdgeIdentity<int>]", to_string(EFC2));
  // Reset ID's for next test
  CurrAddTwoEfId = 0;
}

TEST(EdgeFunctionComposerTest, HandleEFComposition) {
  // ((3 + 2) * 2) + 2
  int InitialValue = 3;
  EdgeFunction<int> AddEF1 = AddTwoEF{++CurrAddTwoEfId};
  EdgeFunction<int> AddEF2 = AddTwoEF{++CurrAddTwoEfId};
  EdgeFunction<int> MulEF = MulTwoEF{++CurrMulTwoEfId};
  auto ComposedEF = (AddEF1.composeWith(MulEF)).composeWith(AddEF2);
  llvm::outs() << "Compose: " << ComposedEF << '\n';
  int Result = ComposedEF.computeTarget(InitialValue);
  llvm::outs() << "Result: " << Result << '\n';
  EXPECT_EQ(12, Result);
  EXPECT_EQ(AddEF1, AddEF2) << "The IDs are not being compared";
}
