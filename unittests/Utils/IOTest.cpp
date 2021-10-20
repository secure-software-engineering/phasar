#include "gtest/gtest.h"

#include "../TestUtils/TestConfig.h"
#include "phasar/Utils/IO.h"

//===----------------------------------------------------------------------===//
// Unit tests for the IO functionalities

TEST(IO, ReadTextFile) {
  std::string File("test.txt");
  std::string Path = psr::unittest::PathToTxtTestFiles + File;
  std::string Contents = psr::readTextFile(Path);
  std::string Expected =
      R"(This is a test file that contains very interesting content.
I am not kidding, this file is subject to an IO test.
Check out the cool project yourself.)";
  ASSERT_EQ(Contents, Expected);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
