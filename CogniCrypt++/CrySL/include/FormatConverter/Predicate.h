#pagma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <memory>
#include <phasar/PhasarLLVM>
#include <ObjectConverter.h>

#include <string>

namespace CCPP {
class Predicate {
  private:
  bool operator==(const Predicate &pc);
  std::string getFunctionName();
  std::vector<Object> params;
};
} // namespace CCPP