#ifndef UNITTEST_TESTUTILS_TESTCONFIG_H_
#define UNITTEST_TESTUTILS_TESTCONFIG_H_

#include "config.h"
#include "phasar/Config/Configuration.h"

#include <string>

namespace psr::unittest {

inline PSR_CONST_CONSTEXPR std::string
    PathToLLTestFiles(PHASAR_BUILD_DIR "/test/llvm_test_code/");
inline PSR_CONST_CONSTEXPR std::string
    PathToJSONTestFiles(PHASAR_SRC_DIR "/test/json_test_code/");

} // namespace psr::unittest

#endif // UNITTEST_TESTUTILS_TESTCONFIG_H_
