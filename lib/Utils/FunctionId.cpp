#include "phasar/Utils/FunctionId.h"

#include <string>

std::string psr::to_string(FunctionId FId) {
  return "@" + std::to_string(size_t(FId));
}
