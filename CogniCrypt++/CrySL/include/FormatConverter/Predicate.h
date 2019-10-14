#pagma once
#include <FormatConverter/DefinedObject.h>
#include <Parser/CrySLParser.h>
#include <set>
#include <string>
#include <vector>

namespace CCPP {
class Predicate {
private:
  std::vector<DefinedObject> params;
  std::string functionName;

public:
  bool operator == (const Predicate &pc);

  void setFunctionName(const std::string &functionName) {this->functionName = functionName;  }
  const std::string &getFunctionName() const { return functionName; }

  void setParams(const std::vector<ObjectWithOutLLVM> &params) {
    this->params = params
  }
  const std::vector<ObjectWithOutLLVM> &getparams() const { return params; }
};

} // namespace CCPP