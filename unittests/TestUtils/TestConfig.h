#ifndef UNITTEST_TESTUTILS_TESTCONFIG_H_
#define UNITTEST_TESTUTILS_TESTCONFIG_H_

#include "config.h"

#include <string>

namespace psr::unittest {

inline const std::string PathToLLTestFiles(PHASAR_BUILD_DIR
                                           "/test/llvm_test_code/");

} // namespace psr::unittest

#endif // UNITTEST_TESTUTILS_TESTCONFIG_H_
