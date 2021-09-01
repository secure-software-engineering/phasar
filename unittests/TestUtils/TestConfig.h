#ifndef UNITTEST_TESTUTILS_TESTCONFIG_H_
#define UNITTEST_TESTUTILS_TESTCONFIG_H_

#include <string>

#include "phasar/Config/Configuration.h"

namespace psr::unittest {

inline const std::string PathToLLTestFiles(PhasarConfig::PhasarDirectory() +
                                           "build/test/llvm_test_code/");

inline const std::string PathToTxtTestFiles(PhasarConfig::PhasarDirectory() +
                                            "build/test/text_test_code/");

inline const std::string PathToJSONTestFiles(PhasarConfig::PhasarDirectory() +
                                             "test/json_test_code/");

} // namespace psr::unittest

#endif // UNITTEST_TESTUTILS_TESTCONFIG_H_
