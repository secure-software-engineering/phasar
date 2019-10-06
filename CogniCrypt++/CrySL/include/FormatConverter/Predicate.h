#pagma once
#include <Parser/CrySLParser.h>
#include <memory>
#include <phasar/PhasarLLVM>
#include <FormatConverter/ObjectConverter.h>
#include <string>
#include <FormatConverter/ObjectWithOutLLVM.h>


namespace CCPP {
class Predicate {
  private:
  bool operator==(const Predicate &pc);
  std::string getFunctionName();
  //std::vector<Object> params;
  std::vector<ObjectWithOutLLVM> params;
};
} // namespace CCPP