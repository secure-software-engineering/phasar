#ifndef UNITTEST_TESTUTILS_TESTCONFIG_H_
#define UNITTEST_TESTUTILS_TESTCONFIG_H_

#include "phasar/Config/Configuration.h"

#include "gtest/gtest.h"

#include <string>

namespace psr::unittest {

static constexpr llvm::StringLiteral PathToLLTestFiles =
    PHASAR_DIR "/build/test/llvm_test_code/";

static constexpr llvm::StringLiteral PathToTxtTestFiles =
    PHASAR_DIR "/build/test/text_test_code/";

static constexpr llvm::StringLiteral PathToJSONTestFiles =
    PHASAR_DIR "/test/json_test_code/";

#define PHASAR_BUILD_SUBFOLDER(SUB)                                            \
  llvm::StringLiteral(PHASAR_DIR "/build/test/llvm_test_code/" SUB)

inline const std::string PathToSwiftTestFiles(
    PhasarConfig::PhasarDirectory() + "build/test/llvm_swift_test_code/");

// Remove wrapped tests in case GTEST_SKIP is not available. This is needed as
// LLVM currently ships with an older version of gtest (<1.10.0) that does not
// support GTEST_SKIP. TODO: Remove this macro after LLVM updated their gtest.
#ifdef GTEST_SKIP
#define PHASAR_SKIP_TEST(...) __VA_ARGS__
#else
#define PHASAR_SKIP_TEST(...)
#endif

#ifdef _LIBCPP_VERSION
#define LIBCPP_GTEST_SKIP GTEST_SKIP();
#else
#define LIBCPP_GTEST_SKIP
#endif

} // namespace psr::unittest

#endif // UNITTEST_TESTUTILS_TESTCONFIG_H_
