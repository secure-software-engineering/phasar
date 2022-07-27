#include "gtest/gtest.h"

#include "phasar/Utils/IO.h"
#include "TestConfig.h"

//===----------------------------------------------------------------------===//
// Unit tests for the IO functionalities

TEST(IO, ReadTextFile) {
  std::string File("test.txt");
  std::string Path = File;

  std::string Contents = psr::readTextFile(Path);
  std::string Expected =
      R"(This is a test file that contains very interesting content.
I am not kidding, this file is subject to an IO test.
Check out the cool project yourself.
)";
  ASSERT_EQ(Contents, Expected);
}
