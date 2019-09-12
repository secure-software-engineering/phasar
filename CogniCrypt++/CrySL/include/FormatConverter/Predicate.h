#pagma once
#include <Parser/CrySLParser.h>
#include <memory>
#include <phasar/PhasarLLVM>
#include <FormatConverter/ObjectConverter.h>

#include <string>

namespace CCPP {
class Predicate {
  private:
  bool operator==(const Predicate &pc);
  std::string getFunctionName();
  std::vector<Object> params;
};
} // namespace CCPP