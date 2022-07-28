#ifndef UNITTEST_TESTUTILS_TESTCONFIG_H_
#define UNITTEST_TESTUTILS_TESTCONFIG_H_

#include <string>

#include "phasar/Config/Configuration.h"

namespace psr::unittest {

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
