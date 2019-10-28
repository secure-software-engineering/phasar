#pagma once
#include <FormatConverter/DefinedObject.h>
#include <Parser/CrySLParser.h>
#include <set>
#include <string>
#include <vector>

namespace CCPP {
class Predicate {
private:
  std::vector<std::unique_ptr<DefinedObject>> params;
  std::string functionName;

public:
  bool operator==(const Predicate &pc) const;

  void setFunctionName(const std::string &functionName) {
    this->functionName = functionName;
  }
  const std::string &getFunctionName() const { return functionName; }

  /*void setParams(const std::vector<DefinedObject> &params) {
    this->params = params
  }*/

  void setParams(std::vector<std::unique_ptr<DefinedObject>> &&params) {
    this->params = std::move(params);
  }

  // const std::vector<DefinedObject> &getparams() const { return params; }
  const std::vector<std::unique_ptr<DefinedObject>> &getParams() const {
    return params;
  }
};

} // namespace CCPP